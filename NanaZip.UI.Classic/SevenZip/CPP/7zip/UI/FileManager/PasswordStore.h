// PasswordStore.h

#ifndef __PASSWORD_STORE_H
#define __PASSWORD_STORE_H

#include "../../../Common/MyString.h"

// **************** NanaZip Modification Start ****************
// Persistent password store for archive passwords (WinRAR-like).
//
// Storage model:
//   - A single file holds a serialized list of archive passwords.
//   - Optional master password:
//       - empty (default): payload is protected with Windows DPAPI (user scope)
//       - set: payload is encrypted with PBKDF2-HMAC-SHA256 (derived key)
//              + AES-256-GCM
//   - File location: program directory (portable), or
//     %LocalAppData%\NanaZip\passwords.dat fallback (MSIX / read-only dir).
//   - Writes are atomic (temp file + MoveFileEx) and guarded by a mutex.
//   - A corrupted/undecryptable store is quarantined as ".bak" and a new
//     empty store is created, so the user is never blocked.
//
// Conventions:
//   - Caller must hold no locks; all functions are thread safe.
//   - "Unlocked" means the plaintext password list is cached in memory for
//     the current session (DPAPI mode is always unlocked implicitly).

namespace NPasswordStore
{
  // True if the store uses a master password (reads only the file header,
  // no unlock required). False for missing store or DPAPI mode.
  bool HasMasterPassword();

  // True if the plaintext list is available for the current session.
  bool IsUnlocked();

  // Drops the in-memory plaintext cache (master password mode).
  void Lock();

  // Unlocks the store for this session. In DPAPI mode any (including
  // empty) master password succeeds. Returns false for a wrong password.
  bool Unlock(const UString &masterPassword);

  // Re-encrypts the current list with a new master password (empty = back
  // to DPAPI mode). Requires an unlocked session.
  bool ChangeMasterPassword(const UString &newMasterPassword);

  // The following functions require an unlocked session; they return
  // false and keep the list untouched otherwise.

  bool GetPasswords(UStringVector &passwords);

  // Adds the password at the front (MRU), removing any duplicate and
  // trimming the list to the maximum size.
  bool AddPassword(const UString &password);

  bool RemovePassword(unsigned index);

  bool ClearPasswords();
}

// **************** NanaZip Modification End ****************

#endif
