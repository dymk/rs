#pragma once

#include <array>

#include "syscall_constants.h"

enum class Fmt {
  DEFAULT,
  FD,
  FD_IO,
  FD_2,
  FTRUNC,
  TRUNC,
  OPEN,
  ACCESS,
  CHMOD,
  FCHMOD,
  CHMOD_EXT,
  FCHMOD_EXT,
  CHFLAGS,
  FCHFLAGS,
  UNMAP_INFO,
  HFS_update,
  AT,
  CHMODAT,
  OPENAT,
  RENAMEAT,
};

struct bsd_syscall {
  static_assert(static_cast<int>(Fmt::DEFAULT) == 0, "bsd_syscall should be all zeros by default");
  const char *sc_name = nullptr;
  Fmt sc_format = Fmt::DEFAULT;
};

static constexpr int MAX_BSD_SYSCALL = 526;

std::array<bsd_syscall, MAX_BSD_SYSCALL> make_bsd_syscall_table() {
  static const std::tuple<int, const char *, Fmt> bsd_syscall_table[] = {
    { BSC_stat, "stat", Fmt::DEFAULT },
    { BSC_stat64, "stat64", Fmt::DEFAULT },
    { BSC_stat_extended, "stat_extended", Fmt::DEFAULT },
    { BSC_stat64_extended, "stat_extended64", Fmt::DEFAULT },
    { BSC_execve, "execve", Fmt::DEFAULT },
    { BSC_posix_spawn, "posix_spawn", Fmt::DEFAULT },
    { BSC_open, "open", Fmt::OPEN },
    { BSC_open_nocancel, "open", Fmt::OPEN },
    { BSC_open_extended, "open_extended", Fmt::OPEN },
    { BSC_guarded_open_np, "guarded_open_np", Fmt::OPEN },
    { BSC_open_dprotected_np, "open_dprotected", Fmt::OPEN },
    { BSC_fstat, "fstat", Fmt::FD },  // TODO(peck): Handle this as read metadata
    { BSC_fstat64, "fstat64", Fmt::FD },  // TODO(peck): Handle this as read metadata
    { BSC_fstat_extended, "fstat_extended", Fmt::FD },  // TODO(peck): Handle this as read metadata
    { BSC_fstat64_extended, "fstat64_extended", Fmt::FD },  // TODO(peck): Handle this as read metadata
    { BSC_lstat, "lstat", Fmt::DEFAULT },
    { BSC_lstat64, "lstat64", Fmt::DEFAULT },
    { BSC_lstat_extended, "lstat_extended", Fmt::DEFAULT },
    { BSC_lstat64_extended, "lstat_extended64", Fmt::DEFAULT },
    { BSC_link, "link", Fmt::DEFAULT },
    { BSC_unlink, "unlink", Fmt::DEFAULT },
    { BSC_mknod, "mknod", Fmt::DEFAULT },
    { BSC_chmod, "chmod", Fmt::CHMOD },
    { BSC_chmod_extended, "chmod_extended", Fmt::CHMOD_EXT },
    { BSC_fchmod, "fchmod", Fmt::FCHMOD },
    { BSC_fchmod_extended, "fchmod_extended", Fmt::FCHMOD_EXT },
    { BSC_chown, "chown", Fmt::DEFAULT },
    { BSC_lchown, "lchown", Fmt::DEFAULT },
    { BSC_fchown, "fchown", Fmt::FD },  // TODO(peck): Handle this as write metadata
    { BSC_access, "access", Fmt::ACCESS },
    { BSC_access_extended, "access_extended", Fmt::DEFAULT },
    { BSC_chdir, "chdir", Fmt::DEFAULT },
    { BSC_pthread_chdir, "pthread_chdir", Fmt::DEFAULT },
    { BSC_chroot, "chroot", Fmt::DEFAULT },
    { BSC_utimes, "utimes", Fmt::DEFAULT },
    { BSC_delete, "delete-Carbon", Fmt::DEFAULT },
    { BSC_undelete, "undelete", Fmt::DEFAULT },
    { BSC_chflags, "chflags", Fmt::CHFLAGS },
    { BSC_fchflags, "fchflags", Fmt::FCHFLAGS },
    { BSC_fchdir, "fchdir", Fmt::FD },
    { BSC_pthread_fchdir, "pthread_fchdir", Fmt::FD },
    { BSC_futimes, "futimes", Fmt::FD },  // TODO(peck): Handle this as write metadata
    { BSC_symlink, "symlink", Fmt::DEFAULT },
    { BSC_readlink, "readlink", Fmt::DEFAULT },
    { BSC_mkdir, "mkdir", Fmt::DEFAULT },
    { BSC_mkdir_extended, "mkdir_extended", Fmt::DEFAULT },
    { BSC_mkfifo, "mkfifo", Fmt::DEFAULT },
    { BSC_mkfifo_extended, "mkfifo_extended", Fmt::DEFAULT },
    { BSC_rmdir, "rmdir", Fmt::DEFAULT },
    { BSC_getdirentries, "getdirentries", Fmt::FD_IO },
    { BSC_getdirentries64, "getdirentries64", Fmt::FD_IO },
    { BSC_truncate, "truncate", Fmt::TRUNC },
    { BSC_ftruncate, "ftruncate", Fmt::FTRUNC },
    { BSC_getattrlist, "getattrlist", Fmt::DEFAULT },
    { BSC_setattrlist, "setattrlist", Fmt::DEFAULT },
    { BSC_fgetattrlist, "fgetattrlist", Fmt::FD },
    { BSC_fsetattrlist, "fsetattrlist", Fmt::FD },
    { BSC_getdirentriesattr, "getdirentriesattr", Fmt::FD },  // TODO(peck):: Handle this as read metadata
    { BSC_exchangedata, "exchangedata", Fmt::DEFAULT },
    { BSC_rename, "rename", Fmt::DEFAULT },
    { BSC_copyfile, "copyfile", Fmt::DEFAULT },
    { BSC_checkuseraccess, "checkuseraccess", Fmt::DEFAULT },
    { BSC_searchfs, "searchfs", Fmt::DEFAULT },
    { BSC_getattrlistbulk, "getattrlistbulk", Fmt::DEFAULT },
    { BSC_openat, "openat", Fmt::OPENAT },
    { BSC_openat_nocancel, "openat", Fmt::OPENAT },
    { BSC_renameat, "renameat", Fmt::RENAMEAT },
    { BSC_chmodat, "chmodat", Fmt::CHMODAT },
    { BSC_chownat, "chownat", Fmt::AT },
    { BSC_fstatat, "fstatat", Fmt::AT },
    { BSC_fstatat64, "fstatat64", Fmt::AT },
    { BSC_linkat, "linkat", Fmt::AT },
    { BSC_unlinkat, "unlinkat", Fmt::AT },
    { BSC_readlinkat, "readlinkat", Fmt::AT },
    { BSC_symlinkat, "symlinkat", Fmt::AT },
    { BSC_mkdirat, "mkdirat", Fmt::AT },
    { BSC_getattrlistat, "getattrlistat", Fmt::AT },
  };

  std::array<bsd_syscall, MAX_BSD_SYSCALL> result;
  for (auto syscall_descriptor : bsd_syscall_table) {
    int code = BSC_INDEX(std::get<0>(syscall_descriptor));

    auto &syscall = result.at(code);
    syscall.sc_name = std::get<1>(syscall_descriptor);
    syscall.sc_format = std::get<2>(syscall_descriptor);
  }
  return result;
}

#define MAX_FILEMGR 512

int filemgr_index(int type) {
  if (type & 0x10000) {
    return (((type >> 2) & 0x3fff) + 256);
  }

  return (((type >> 2) & 0x3fff));
}

struct filemgr_call {
  const char *fm_name = nullptr;
};

std::array<filemgr_call, MAX_FILEMGR> make_filemgr_calls() {
  static const std::pair<int, const char *> filemgr_call_types[] = {
    { FILEMGR_PBGETCATALOGINFO, "GetCatalogInfo" },
    { FILEMGR_PBGETCATALOGINFOBULK, "GetCatalogInfoBulk" },
    { FILEMGR_PBCREATEFILEUNICODE, "CreateFileUnicode" },
    { FILEMGR_PBCREATEDIRECTORYUNICODE, "CreateDirectoryUnicode" },
    { FILEMGR_PBCREATEFORK, "PBCreateFork" },
    { FILEMGR_PBDELETEFORK, "PBDeleteFork" },
    { FILEMGR_PBITERATEFORK, "PBIterateFork" },
    { FILEMGR_PBOPENFORK, "PBOpenFork" },
    { FILEMGR_PBREADFORK, "PBReadFork" },
    { FILEMGR_PBWRITEFORK, "PBWriteFork" },
    { FILEMGR_PBALLOCATEFORK, "PBAllocateFork" },
    { FILEMGR_PBDELETEOBJECT, "PBDeleteObject" },
    { FILEMGR_PBEXCHANGEOBJECT, "PBExchangeObject" },
    { FILEMGR_PBGETFORKCBINFO, "PBGetForkCBInfo" },
    { FILEMGR_PBGETVOLUMEINFO, "PBGetVolumeInfo" },
    { FILEMGR_PBMAKEFSREF, "PBMakeFSRef" },
    { FILEMGR_PBMAKEFSREFUNICODE, "PBMakeFSRefUnicode" },
    { FILEMGR_PBMOVEOBJECT, "PBMoveObject" },
    { FILEMGR_PBOPENITERATOR, "PBOpenIterator" },
    { FILEMGR_PBRENAMEUNICODE, "PBRenameUnicode" },
    { FILEMGR_PBSETCATALOGINFO, "SetCatalogInfo" },
    { FILEMGR_PBSETVOLUMEINFO, "SetVolumeInfo" },
    { FILEMGR_FSREFMAKEPATH, "FSRefMakePath" },
    { FILEMGR_FSPATHMAKEREF, "FSPathMakeRef" },
    { FILEMGR_PBGETCATINFO, "GetCatInfo" },
    { FILEMGR_PBGETCATINFOLITE, "GetCatInfoLite" },
    { FILEMGR_PBHGETFINFO, "PBHGetFInfo" },
    { FILEMGR_PBXGETVOLINFO, "PBXGetVolInfo" },
    { FILEMGR_PBHCREATE, "PBHCreate" },
    { FILEMGR_PBHOPENDF, "PBHOpenDF" },
    { FILEMGR_PBHOPENRF, "PBHOpenRF" },
    { FILEMGR_PBHGETDIRACCESS, "PBHGetDirAccess" },
    { FILEMGR_PBHSETDIRACCESS, "PBHSetDirAccess" },
    { FILEMGR_PBHMAPID, "PBHMapID" },
    { FILEMGR_PBHMAPNAME, "PBHMapName" },
    { FILEMGR_PBCLOSE, "PBClose" },
    { FILEMGR_PBFLUSHFILE, "PBFlushFile" },
    { FILEMGR_PBGETEOF, "PBGetEOF" },
    { FILEMGR_PBSETEOF, "PBSetEOF" },
    { FILEMGR_PBGETFPOS, "PBGetFPos" },
    { FILEMGR_PBREAD, "PBRead" },
    { FILEMGR_PBWRITE, "PBWrite" },
    { FILEMGR_PBGETFCBINFO, "PBGetFCBInfo" },
    { FILEMGR_PBSETFINFO, "PBSetFInfo" },
    { FILEMGR_PBALLOCATE, "PBAllocate" },
    { FILEMGR_PBALLOCCONTIG, "PBAllocContig" },
    { FILEMGR_PBSETFPOS, "PBSetFPos" },
    { FILEMGR_PBSETCATINFO, "PBSetCatInfo" },
    { FILEMGR_PBGETVOLPARMS, "PBGetVolParms" },
    { FILEMGR_PBSETVINFO, "PBSetVInfo" },
    { FILEMGR_PBMAKEFSSPEC, "PBMakeFSSpec" },
    { FILEMGR_PBHGETVINFO, "PBHGetVInfo" },
    { FILEMGR_PBCREATEFILEIDREF, "PBCreateFileIDRef" },
    { FILEMGR_PBDELETEFILEIDREF, "PBDeleteFileIDRef" },
    { FILEMGR_PBRESOLVEFILEIDREF, "PBResolveFileIDRef" },
    { FILEMGR_PBFLUSHVOL, "PBFlushVol" },
    { FILEMGR_PBHRENAME, "PBHRename" },
    { FILEMGR_PBCATMOVE, "PBCatMove" },
    { FILEMGR_PBEXCHANGEFILES, "PBExchangeFiles" },
    { FILEMGR_PBHDELETE, "PBHDelete" },
    { FILEMGR_PBDIRCREATE, "PBDirCreate" },
    { FILEMGR_PBCATSEARCH, "PBCatSearch" },
    { FILEMGR_PBHSETFLOCK, "PBHSetFlock" },
    { FILEMGR_PBHRSTFLOCK, "PBHRstFLock" },
    { FILEMGR_PBLOCKRANGE, "PBLockRange" },
    { FILEMGR_PBUNLOCKRANGE, "PBUnlockRange" }
  };

  std::array<filemgr_call, MAX_FILEMGR> result{};
  for (auto filemgr_call_type : filemgr_call_types) {
    int code = filemgr_index(filemgr_call_type.first);
    result[code].fm_name = filemgr_call_type.second;
  }
  return result;
}
