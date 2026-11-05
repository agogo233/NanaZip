// PasswordStore.cpp

#include "StdAfx.h"

#include "PasswordStore.h"

// **************** NanaZip Modification Start ****************

#include <windows.h>
#include <dpapi.h>
#include <bcrypt.h>
#include <appmodel.h>

#include "../../../Common/MyBuffer.h"
#include "../../../Windows/DLL.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileFind.h"
#include "../../../Windows/FileIO.h"
#include "../../../Windows/Synchronization.h"

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "Crypt32.lib")

using namespace NWindows;

namespace NPasswordStore {

static const Byte kMagic[8] = { 'N', 'Z', 'P', 'S', '0', '0', '0', '1' };

static const unsigned kEncTypeDpapi = 0;
static const unsigned kEncTypeMaster = 1;

static const size_t kSaltSize = 16;
static const size_t kNonceSize = 12;
static const size_t kTagSize = 16;
static const size_t kKeySize = 32;
static const UInt32 kPbkdf2Iterations = 600000;

static const unsigned kMaxPasswords = 32;
static const size_t kMaxPasswordBytes = 4096;
static const size_t kMaxFileBytes = (size_t)1 << 20;

static NSynchronization::CCriticalSection g_StoreCS;

static bool g_sessionUnlocked = false;
static bool g_masterMode = false;
static UStringVector g_passwords;

// Cached key material for the master password mode. Only the derived key
// is kept (never the master password text), so "Lock" can fully drop it.
static bool g_keyCached = false;
static Byte g_cachedKey[kKeySize];
static Byte g_cachedSalt[kSaltSize];
static UInt32 g_cachedIterations = kPbkdf2Iterations;

static void PutUInt32(Byte *p, UInt32 v)
{
  for (unsigned i = 0; i < 4; i++)
    p[i] = (Byte)(v >> (8 * i));
}

static UInt32 GetUInt32(const Byte *p)
{
  UInt32 v = 0;
  for (unsigned i = 0; i < 4; i++)
    v |= ((UInt32)p[i]) << (8 * i);
  return v;
}

static void WipeCachedKey()
{
  SecureZeroMemory(g_cachedKey, sizeof(g_cachedKey));
  SecureZeroMemory(g_cachedSalt, sizeof(g_cachedSalt));
  g_keyCached = false;
}

// ---------- password list serialization ----------

static bool SerializePasswords(const UStringVector &items, CByteBuffer_Wipe &out)
{
  size_t total = 4;
  for (unsigned i = 0; i < items.Size(); i++)
  {
    const size_t len = (size_t)items[i].Len() * sizeof(wchar_t);
    if (len > kMaxPasswordBytes)
      return false;
    if (total + 4 + len > kMaxFileBytes)
      return false;
    total += 4 + len;
  }
  out.Alloc(total);
  Byte *p = out;
  PutUInt32(p, (UInt32)items.Size());
  p += 4;
  for (unsigned i = 0; i < items.Size(); i++)
  {
    const size_t len = (size_t)items[i].Len() * sizeof(wchar_t);
    PutUInt32(p, (UInt32)len);
    p += 4;
    memcpy(p, items[i].Ptr(), len);
    p += len;
  }
  return true;
}

static bool DeserializePasswords(const Byte *data, size_t size, UStringVector &items)
{
  items.Clear();
  if (size < 4)
    return false;
  const UInt32 count = GetUInt32(data);
  if (count > kMaxPasswords)
    return false;
  const Byte *p = data + 4;
  size_t rem = size - 4;
  for (UInt32 i = 0; i < count; i++)
  {
    if (rem < 4)
      return false;
    const UInt32 len = GetUInt32(p);
    if (len > kMaxPasswordBytes || (size_t)len > rem - 4 || len % sizeof(wchar_t) != 0)
      return false;
    UString s;
    s.SetFrom((const wchar_t *)(p + 4), (unsigned)(len / sizeof(wchar_t)));
    items.Add(s);
    p += 4 + len;
    rem -= 4 + len;
  }
  return rem == 0;
}

// ---------- DPAPI ----------

static bool EncryptWithDpapi(const Byte *data, size_t size, CByteBuffer_Wipe &out)
{
  DATA_BLOB in;
  in.cbData = (DWORD)size;
  in.pbData = (BYTE *)data;
  DATA_BLOB enc;
  if (!::CryptProtectData(&in, L"NanaZip.Passwords", NULL, NULL, NULL, 0, &enc))
    return false;
  out.CopyFrom(enc.pbData, enc.cbData);
  ::LocalFree(enc.pbData);
  return true;
}

static bool DecryptWithDpapi(const Byte *data, size_t size, CByteBuffer_Wipe &out)
{
  DATA_BLOB in;
  in.cbData = (DWORD)size;
  in.pbData = (BYTE *)data;
  DATA_BLOB dec;
  if (!::CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &dec))
    return false;
  out.CopyFrom(dec.pbData, dec.cbData);
  SecureZeroMemory(dec.pbData, dec.cbData);
  ::LocalFree(dec.pbData);
  return true;
}

// ---------- PBKDF2 + AES-256-GCM ----------

static bool DeriveKey(const UString &master, const Byte *salt, UInt32 iterations, Byte *key)
{
  BCRYPT_ALG_HANDLE hAlg = NULL;
  NTSTATUS st = ::BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
  if (!BCRYPT_SUCCESS(st))
    return false;
  st = ::BCryptDeriveKeyPBKDF2(
      hAlg,
      (PUCHAR)master.Ptr(), (ULONG)(master.Len() * sizeof(wchar_t)),
      (PUCHAR)salt, (ULONG)kSaltSize,
      (ULONGLONG)iterations,
      (PUCHAR)key, (ULONG)kKeySize,
      0);
  ::BCryptCloseAlgorithmProvider(hAlg, 0);
  return BCRYPT_SUCCESS(st);
}

static bool AesGcmCrypt(bool encrypt, const Byte *key, const Byte *nonce,
    const Byte *data, size_t dataSize, Byte *out, Byte *tag)
{
  BCRYPT_ALG_HANDLE hAlg = NULL;
  NTSTATUS st = ::BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
  if (!BCRYPT_SUCCESS(st))
    return false;
  st = ::BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
      (PUCHAR)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
  if (!BCRYPT_SUCCESS(st))
  {
    ::BCryptCloseAlgorithmProvider(hAlg, 0);
    return false;
  }
  BCRYPT_KEY_HANDLE hKey = NULL;
  st = ::BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, (PUCHAR)key, (ULONG)kKeySize, 0);
  if (!BCRYPT_SUCCESS(st))
  {
    ::BCryptCloseAlgorithmProvider(hAlg, 0);
    return false;
  }
  BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
  BCRYPT_INIT_AUTH_MODE_INFO(info);
  info.pbNonce = (PUCHAR)nonce;
  info.cbNonce = (ULONG)kNonceSize;
  info.pbTag = tag;
  info.cbTag = (ULONG)kTagSize;
  ULONG resultSize = 0;
  if (encrypt)
    st = ::BCryptEncrypt(hKey, (PUCHAR)data, (ULONG)dataSize, &info,
        NULL, 0, out, (ULONG)dataSize, &resultSize, 0);
  else
    st = ::BCryptDecrypt(hKey, (PUCHAR)data, (ULONG)dataSize, &info,
        NULL, 0, out, (ULONG)dataSize, &resultSize, 0);
  ::BCryptDestroyKey(hKey);
  ::BCryptCloseAlgorithmProvider(hAlg, 0);
  return BCRYPT_SUCCESS(st) && resultSize == dataSize;
}

static bool GenerateRandom(Byte *data, size_t size)
{
  return BCRYPT_SUCCESS(::BCryptGenRandom(NULL, (PUCHAR)data, (ULONG)size, BCRYPT_USE_SYSTEM_PREFERRED_RNG));
}

// ---------- store file path ----------

static bool IsRunningInMsix()
{
  wchar_t name[128];
  UINT32 size = ARRAY_SIZE(name);
  return ::GetCurrentPackageFullName(&size, name) == ERROR_SUCCESS;
}

static bool GetLocalAppDataStorePath(FString &path)
{
  wchar_t buf[MAX_PATH + 64];
  const DWORD len = ::GetEnvironmentVariableW(L"LOCALAPPDATA", buf, ARRAY_SIZE(buf));
  if (len == 0 || len >= ARRAY_SIZE(buf))
    return false;
  FString dir(buf);
  dir.Add_PathSepar();
  dir += L"NanaZip";
  NWindows::NFile::NDir::CreateComplexDir(dir);
  path = dir;
  path.Add_PathSepar();
  path += L"passwords.dat";
  return true;
}

static bool ProbeWritable(const FString &path)
{
  FString probe = path;
  probe += L".probe";
  {
    NWindows::NFile::NIO::COutFile file;
    if (!file.Create(probe, true))
      return false;
  }
  ::DeleteFileW(probe);
  return true;
}

// g_StoreCS must be held.
static bool ResolveStorePath(FString &path)
{
  static FString s_path;
  static bool s_resolved = false;
  if (s_resolved)
  {
    path = s_path;
    return true;
  }
  bool resolved = false;
  if (!IsRunningInMsix())
  {
    FString candidate = NWindows::NDLL::GetModuleDirPrefix();
    candidate += L"NanaZip.passwords";
    if (ProbeWritable(candidate))
    {
      s_path = candidate;
      resolved = true;
    }
  }
  if (!resolved)
  {
    if (!GetLocalAppDataStorePath(s_path))
      return false;
  }
  s_resolved = true;
  path = s_path;
  return true;
}

// ---------- file io ----------

static bool LoadFileToBuffer(const FString &path, CByteBuffer &buf)
{
  NWindows::NFile::NIO::CInFile file;
  if (!file.Open(path))
    return false;
  UInt64 length;
  if (!file.GetLength(length) || length < 9 || length > kMaxFileBytes)
    return false;
  buf.Alloc((size_t)length);
  UInt32 processed = 0;
  if (!file.Read(buf, (UInt32)length, processed) || processed != length)
    return false;
  return true;
}

static bool SaveBufferToFile(const FString &path, const Byte *data, size_t size)
{
  FString tmp = path;
  tmp += L".tmp";
  {
    NWindows::NFile::NIO::COutFile file;
    if (!file.Create(tmp, true))
      return false;
    UInt32 processed = 0;
    if (!file.Write(data, (UInt32)size, processed) || processed != size)
    {
      file.Close();
      ::DeleteFileW(tmp);
      return false;
    }
  }
  if (!::MoveFileExW(tmp, path, MOVEFILE_REPLACE_EXISTING))
  {
    ::DeleteFileW(tmp);
    return false;
  }
  return true;
}

// ---------- store build/parse ----------

// g_StoreCS must be held. Uses the cached key with a fresh nonce.
static bool BuildStoreWithKey(const UStringVector &items, CByteBuffer &fileData)
{
  CByteBuffer_Wipe plain(0);
  if (!SerializePasswords(items, plain))
    return false;
  Byte nonce[kNonceSize];
  Byte tag[kTagSize];
  if (!GenerateRandom(nonce, kNonceSize))
    return false;
  CByteBuffer_Wipe cipher(0);
  cipher.Alloc(plain.Size());
  if (!AesGcmCrypt(true, g_cachedKey, nonce, plain, plain.Size(), cipher, tag))
    return false;
  const size_t total = 9 + kSaltSize + 4 + kNonceSize + 4 + cipher.Size() + kTagSize;
  fileData.Alloc(total);
  Byte *p = fileData;
  memcpy(p, kMagic, 8); p += 8;
  *p++ = kEncTypeMaster;
  memcpy(p, g_cachedSalt, kSaltSize); p += kSaltSize;
  PutUInt32(p, g_cachedIterations); p += 4;
  memcpy(p, nonce, kNonceSize); p += kNonceSize;
  PutUInt32(p, (UInt32)cipher.Size()); p += 4;
  memcpy(p, cipher, cipher.Size()); p += cipher.Size();
  memcpy(p, tag, kTagSize);
  return true;
}

static bool BuildStoreWithDpapi(const UStringVector &items, CByteBuffer &fileData)
{
  CByteBuffer_Wipe plain(0);
  if (!SerializePasswords(items, plain))
    return false;
  CByteBuffer_Wipe payload(0);
  if (!EncryptWithDpapi(plain, plain.Size(), payload))
    return false;
  const size_t total = 9 + 4 + payload.Size();
  fileData.Alloc(total);
  Byte *p = fileData;
  memcpy(p, kMagic, 8); p += 8;
  *p++ = kEncTypeDpapi;
  PutUInt32(p, (UInt32)payload.Size()); p += 4;
  memcpy(p, payload, payload.Size());
  return true;
}

struct CStoreHeader
{
  unsigned EncType;
  const Byte *Payload;
  size_t PayloadSize;
  const Byte *Salt;
  UInt32 Iterations;
  const Byte *Nonce;
  const Byte *Cipher;
  size_t CipherSize;
  const Byte *Tag;
};

static bool ParseStore(const Byte *data, size_t size, CStoreHeader &h)
{
  if (size < 9)
    return false;
  if (memcmp(data, kMagic, 8) != 0)
    return false;
  h.EncType = data[8];
  if (h.EncType == kEncTypeDpapi)
  {
    if (size < 13)
      return false;
    const UInt32 len = GetUInt32(data + 9);
    if ((size_t)len != size - 13)
      return false;
    h.Payload = data + 13;
    h.PayloadSize = len;
    return true;
  }
  if (h.EncType == kEncTypeMaster)
  {
    const size_t head = 9 + kSaltSize + 4 + kNonceSize + 4;
    if (size < head + kTagSize)
      return false;
    h.Salt = data + 9;
    h.Iterations = GetUInt32(data + 9 + kSaltSize);
    if (h.Iterations == 0 || h.Iterations > 100000000)
      return false;
    h.Nonce = data + 9 + kSaltSize + 4;
    const UInt32 cipherLen = GetUInt32(data + 9 + kSaltSize + 4 + kNonceSize);
    if ((size_t)cipherLen != size - head - kTagSize)
      return false;
    h.Cipher = data + head;
    h.CipherSize = cipherLen;
    h.Tag = data + head + cipherLen;
    return true;
  }
  return false;
}

// g_StoreCS must be held.
static void QuarantineStore(const FString &path)
{
  FString bak = path;
  bak += L".bak";
  ::DeleteFileW(bak);
  ::MoveFileExW(path, bak, MOVEFILE_REPLACE_EXISTING);
}

// g_StoreCS must be held.
static void ResetSessionState(bool masterMode)
{
  g_masterMode = masterMode;
  g_passwords.Clear();
  g_sessionUnlocked = !masterMode;
  if (!masterMode)
    WipeCachedKey();
}

// Loads the store into the session cache (g_StoreCS must be held).
// Returns:
//   0 - cache is ready (missing store, DPAPI mode, or already unlocked)
//   1 - store needs the master password to unlock
//  -1 - internal error (unresolvable path)
static int LoadStoreToCache()
{
  FString path;
  if (!ResolveStorePath(path))
    return -1;
  if (!NWindows::NFile::NFind::DoesFileExist_Raw(path))
  {
    // No store yet: an implicit empty DPAPI store.
    ResetSessionState(false);
    return 0;
  }
  CByteBuffer fileData;
  if (!LoadFileToBuffer(path, fileData))
  {
    QuarantineStore(path);
    ResetSessionState(false);
    return 0;
  }
  CStoreHeader h;
  if (!ParseStore(fileData, fileData.Size(), h))
  {
    QuarantineStore(path);
    ResetSessionState(false);
    return 0;
  }
  if (h.EncType == kEncTypeMaster)
  {
    g_masterMode = true;
    g_sessionUnlocked = false;
    g_passwords.Clear();
    return 1;
  }
  CByteBuffer_Wipe plain(0);
  if (!DecryptWithDpapi(h.Payload, h.PayloadSize, plain) ||
      !DeserializePasswords(plain, plain.Size(), g_passwords))
  {
    // The DPAPI blob is bound to the user account: a reinstalled OS or a
    // migrated account makes it undecryptable. Quarantine and start fresh.
    QuarantineStore(path);
    ResetSessionState(false);
  }
  else
  {
    g_masterMode = false;
    g_sessionUnlocked = true;
    WipeCachedKey();
  }
  return 0;
}

// g_StoreCS must be held.
static bool SaveCacheToStore()
{
  FString path;
  if (!ResolveStorePath(path))
    return false;
  CByteBuffer fileData;
  if (g_masterMode)
  {
    if (!g_keyCached)
      return false;
    if (!BuildStoreWithKey(g_passwords, fileData))
      return false;
  }
  else
  {
    if (!BuildStoreWithDpapi(g_passwords, fileData))
      return false;
  }
  return SaveBufferToFile(path, fileData, fileData.Size());
}

// g_StoreCS must be held.
static bool EnsureUnlocked()
{
  if (g_sessionUnlocked)
    return true;
  return LoadStoreToCache() == 0;
}

// ---------- public api ----------

bool HasMasterPassword()
{
  NSynchronization::CCriticalSectionLock lock(g_StoreCS);
  if (g_sessionUnlocked || g_masterMode)
    return g_masterMode;
  FString path;
  if (!ResolveStorePath(path))
    return false;
  if (!NWindows::NFile::NFind::DoesFileExist_Raw(path))
    return false;
  CByteBuffer fileData;
  if (!LoadFileToBuffer(path, fileData) || fileData.Size() < 9)
    return false;
  if (memcmp(fileData, kMagic, 8) != 0)
    return false;
  return fileData[8] == kEncTypeMaster;
}

bool IsUnlocked()
{
  NSynchronization::CCriticalSectionLock lock(g_StoreCS);
  return g_sessionUnlocked || !g_masterMode;
}

void Lock()
{
  NSynchronization::CCriticalSectionLock lock(g_StoreCS);
  if (g_masterMode)
  {
    g_sessionUnlocked = false;
    g_passwords.Clear();
    WipeCachedKey();
  }
}

bool Unlock(const UString &masterPassword)
{
  NSynchronization::CCriticalSectionLock lock(g_StoreCS);
  if (g_sessionUnlocked)
    return true;
  const int res = LoadStoreToCache();
  if (res == 0)
    return true;
  if (res < 0)
    return false;
  FString path;
  if (!ResolveStorePath(path))
    return false;
  CByteBuffer fileData;
  if (!LoadFileToBuffer(path, fileData))
    return false;
  CStoreHeader h;
  if (!ParseStore(fileData, fileData.Size(), h) || h.EncType != kEncTypeMaster)
    return false;
  Byte key[kKeySize];
  if (!DeriveKey(masterPassword, h.Salt, h.Iterations, key))
    return false;
  CByteBuffer_Wipe plain(0);
  plain.Alloc(h.CipherSize);
  if (!AesGcmCrypt(false, key, h.Nonce, h.Cipher, h.CipherSize, plain, (Byte *)h.Tag))
  {
    SecureZeroMemory(key, sizeof(key));
    return false;
  }
  // Cache the derived key (not the master password text) for this session.
  memcpy(g_cachedKey, key, kKeySize);
  SecureZeroMemory(key, sizeof(key));
  if (!DeserializePasswords(plain, plain.Size(), g_passwords))
    return false;
  memcpy(g_cachedSalt, h.Salt, kSaltSize);
  g_cachedIterations = h.Iterations;
  g_keyCached = true;
  g_sessionUnlocked = true;
  return true;
}

bool ChangeMasterPassword(const UString &newMasterPassword)
{
  NSynchronization::CCriticalSectionLock lock(g_StoreCS);
  if (!EnsureUnlocked())
    return false;
  if (newMasterPassword.IsEmpty())
  {
    g_masterMode = false;
    g_sessionUnlocked = true;
    WipeCachedKey();
  }
  else
  {
    Byte salt[kSaltSize];
    if (!GenerateRandom(salt, kSaltSize))
      return false;
    if (!DeriveKey(newMasterPassword, salt, kPbkdf2Iterations, g_cachedKey))
      return false;
    memcpy(g_cachedSalt, salt, kSaltSize);
    g_cachedIterations = kPbkdf2Iterations;
    g_keyCached = true;
    g_masterMode = true;
    g_sessionUnlocked = true;
  }
  return SaveCacheToStore();
}

bool GetPasswords(UStringVector &passwords)
{
  NSynchronization::CCriticalSectionLock lock(g_StoreCS);
  if (!EnsureUnlocked())
    return false;
  passwords = g_passwords;
  return true;
}

bool AddPassword(const UString &password)
{
  NSynchronization::CCriticalSectionLock lock(g_StoreCS);
  if (!EnsureUnlocked())
    return false;
  for (unsigned i = g_passwords.Size(); i != 0;)
    if (g_passwords[--i] == password)
      g_passwords.Delete(i);
  g_passwords.Insert(0, password);
  if (g_passwords.Size() > kMaxPasswords)
    g_passwords.DeleteFrom(kMaxPasswords);
  return SaveCacheToStore();
}

bool RemovePassword(unsigned index)
{
  NSynchronization::CCriticalSectionLock lock(g_StoreCS);
  if (!EnsureUnlocked())
    return false;
  if (index >= g_passwords.Size())
    return false;
  g_passwords.Delete(index);
  return SaveCacheToStore();
}

bool ClearPasswords()
{
  NSynchronization::CCriticalSectionLock lock(g_StoreCS);
  if (!EnsureUnlocked())
    return false;
  g_passwords.Clear();
  return SaveCacheToStore();
}

} // namespace NPasswordStore

// **************** NanaZip Modification End ****************
