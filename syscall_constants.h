#pragma once

static constexpr int MSC_map_fd = 0x010c00ac;

static constexpr int BSC_BASE = 0x040C0000;
static constexpr int MSC_BASE = 0x010C0000;

// Network related codes
static constexpr int BSC_recvmsg = 0x040C006C;
static constexpr int BSC_sendmsg = 0x040C0070;
static constexpr int BSC_recvfrom = 0x040C0074;
static constexpr int BSC_accept = 0x040C0078;
static constexpr int BSC_select = 0x040C0174;
static constexpr int BSC_socket = 0x040C0184;
static constexpr int BSC_connect = 0x040C0188;
static constexpr int BSC_bind = 0x040C01A0;
static constexpr int BSC_listen = 0x040C01A8;
static constexpr int BSC_sendto = 0x040C0214;
static constexpr int BSC_socketpair = 0x040C021C;
static constexpr int BSC_recvmsg_nocancel = 0x040c0644;
static constexpr int BSC_sendmsg_nocancel = 0x040c0648;
static constexpr int BSC_recvfrom_nocancel = 0x040c064c;
static constexpr int BSC_accept_nocancel = 0x040c0650;
static constexpr int BSC_connect_nocancel = 0x040c0664;
static constexpr int BSC_sendto_nocancel = 0x040c0674;

static constexpr int BSC_exit = 0x040C0004;
static constexpr int BSC_read = 0x040C000C;
static constexpr int BSC_write = 0x040C0010;
static constexpr int BSC_open = 0x040C0014;
static constexpr int BSC_close = 0x040C0018;
static constexpr int BSC_link = 0x040C0024;
static constexpr int BSC_unlink = 0x040C0028;
static constexpr int BSC_chdir = 0x040c0030;
static constexpr int BSC_fchdir = 0x040c0034;
static constexpr int BSC_mknod = 0x040C0038;
static constexpr int BSC_chmod = 0x040C003C;
static constexpr int BSC_chown = 0x040C0040;
static constexpr int BSC_getfsstat = 0x040C0048;
static constexpr int BSC_access = 0x040C0084;
static constexpr int BSC_chflags = 0x040C0088;
static constexpr int BSC_fchflags = 0x040C008C;
static constexpr int BSC_sync = 0x040C0090;
static constexpr int BSC_dup = 0x040C00A4;
static constexpr int BSC_ioctl = 0x040C00D8;
static constexpr int BSC_revoke = 0x040C00E0;
static constexpr int BSC_symlink = 0x040C00E4  ;
static constexpr int BSC_readlink = 0x040C00E8;
static constexpr int BSC_execve = 0x040C00EC;
static constexpr int BSC_umask = 0x040C00F0;
static constexpr int BSC_chroot = 0x040C00F4;
static constexpr int BSC_msync = 0x040C0104;
static constexpr int BSC_dup2 = 0x040C0168;
static constexpr int BSC_fcntl = 0x040C0170;
static constexpr int BSC_fsync = 0x040C017C  ;
static constexpr int BSC_readv = 0x040C01E0  ;
static constexpr int BSC_writev = 0x040C01E4  ;
static constexpr int BSC_fchown = 0x040C01EC  ;
static constexpr int BSC_fchmod = 0x040C01F0  ;
static constexpr int BSC_rename = 0x040C0200;
static constexpr int BSC_flock = 0x040C020C;
static constexpr int BSC_mkfifo = 0x040C0210  ;
static constexpr int BSC_mkdir = 0x040C0220  ;
static constexpr int BSC_rmdir = 0x040C0224;
static constexpr int BSC_utimes = 0x040C0228;
static constexpr int BSC_futimes = 0x040C022C;
static constexpr int BSC_pread = 0x040C0264;
static constexpr int BSC_pwrite = 0x040C0268;
static constexpr int BSC_statfs = 0x040C0274  ;
static constexpr int BSC_fstatfs = 0x040C0278;
static constexpr int BSC_unmount = 0x040C027C;
static constexpr int BSC_mount = 0x040C029C;
static constexpr int BSC_fdatasync = 0x040C02EC;
static constexpr int BSC_stat = 0x040C02F0  ;
static constexpr int BSC_fstat = 0x040C02F4  ;
static constexpr int BSC_lstat = 0x040C02F8  ;
static constexpr int BSC_pathconf = 0x040C02FC  ;
static constexpr int BSC_fpathconf = 0x040C0300;
static constexpr int BSC_getdirentries = 0x040C0310;
static constexpr int BSC_mmap = 0x040c0314;
static constexpr int BSC_lseek = 0x040c031c;
static constexpr int BSC_truncate = 0x040C0320;
static constexpr int BSC_ftruncate = 0x040C0324;
static constexpr int BSC_undelete = 0x040C0334;
static constexpr int BSC_open_dprotected_np = 0x040C0360  ;
static constexpr int BSC_getattrlist = 0x040C0370  ;
static constexpr int BSC_setattrlist = 0x040C0374  ;
static constexpr int BSC_getdirentriesattr = 0x040C0378  ;
static constexpr int BSC_exchangedata = 0x040C037C  ;
static constexpr int BSC_checkuseraccess = 0x040C0380  ;
static constexpr int BSC_searchfs = 0x040C0384;
static constexpr int BSC_delete = 0x040C0388;
static constexpr int BSC_copyfile = 0x040C038C;
static constexpr int BSC_fgetattrlist = 0x040C0390;
static constexpr int BSC_fsetattrlist = 0x040C0394;
static constexpr int BSC_getxattr = 0x040C03A8;
static constexpr int BSC_fgetxattr = 0x040C03AC;
static constexpr int BSC_setxattr = 0x040C03B0;
static constexpr int BSC_fsetxattr = 0x040C03B4;
static constexpr int BSC_removexattr = 0x040C03B8;
static constexpr int BSC_fremovexattr = 0x040C03BC;
static constexpr int BSC_listxattr = 0x040C03C0;
static constexpr int BSC_flistxattr = 0x040C03C4;
static constexpr int BSC_fsctl = 0x040C03C8;
static constexpr int BSC_posix_spawn = 0x040C03D0;
static constexpr int BSC_ffsctl = 0x040C03D4;
static constexpr int BSC_open_extended = 0x040C0454;
static constexpr int BSC_umask_extended = 0x040C0458;
static constexpr int BSC_stat_extended = 0x040C045C;
static constexpr int BSC_lstat_extended = 0x040C0460;
static constexpr int BSC_fstat_extended = 0x040C0464;
static constexpr int BSC_chmod_extended = 0x040C0468;
static constexpr int BSC_fchmod_extended = 0x040C046C;
static constexpr int BSC_access_extended = 0x040C0470;
static constexpr int BSC_mkfifo_extended = 0x040C048C;
static constexpr int BSC_mkdir_extended = 0x040C0490;
static constexpr int BSC_aio_fsync = 0x040C04E4;
static constexpr int BSC_aio_return = 0x040C04E8;
static constexpr int BSC_aio_suspend = 0x040C04EC;
static constexpr int BSC_aio_cancel = 0x040C04F0;
static constexpr int BSC_aio_error = 0x040C04F4;
static constexpr int BSC_aio_read = 0x040C04F8;
static constexpr int BSC_aio_write = 0x040C04FC;
static constexpr int BSC_lio_listio = 0x040C0500;
static constexpr int BSC_sendfile = 0x040C0544;
static constexpr int BSC_stat64 = 0x040C0548;
static constexpr int BSC_fstat64 = 0x040C054C;
static constexpr int BSC_lstat64 = 0x040C0550;
static constexpr int BSC_stat64_extended = 0x040C0554;
static constexpr int BSC_lstat64_extended = 0x040C0558;
static constexpr int BSC_fstat64_extended = 0x040C055C;
static constexpr int BSC_getdirentries64 = 0x040C0560;
static constexpr int BSC_statfs64 = 0x040C0564;
static constexpr int BSC_fstatfs64 = 0x040C0568;
static constexpr int BSC_getfsstat64 = 0x040C056C;
static constexpr int BSC_pthread_chdir = 0x040C0570;
static constexpr int BSC_pthread_fchdir = 0x040C0574;
static constexpr int BSC_lchown = 0x040C05B0;

static constexpr int BSC_read_nocancel = 0x040c0630;
static constexpr int BSC_write_nocancel = 0x040c0634;
static constexpr int BSC_open_nocancel = 0x040c0638;
static constexpr int BSC_close_nocancel = 0x040c063c;
static constexpr int BSC_msync_nocancel = 0x040c0654;
static constexpr int BSC_fcntl_nocancel = 0x040c0658;
static constexpr int BSC_select_nocancel = 0x040c065c;
static constexpr int BSC_fsync_nocancel = 0x040c0660;
static constexpr int BSC_readv_nocancel = 0x040c066c;
static constexpr int BSC_writev_nocancel = 0x040c0670;
static constexpr int BSC_pread_nocancel = 0x040c0678;
static constexpr int BSC_pwrite_nocancel = 0x040c067c;
static constexpr int BSC_aio_suspend_nocancel = 0x40c0694;
static constexpr int BSC_guarded_open_np = 0x040c06e4;
static constexpr int BSC_guarded_close_np = 0x040c06e8;

static constexpr int BSC_fsgetpath = 0x040c06ac;

static constexpr int BSC_getattrlistbulk = 0x040c0734;

static constexpr int BSC_openat = 0x040c073c;
static constexpr int BSC_openat_nocancel = 0x040c0740;
static constexpr int BSC_renameat = 0x040c0744;
static constexpr int BSC_chmodat = 0x040c074c;
static constexpr int BSC_chownat = 0x040c0750;
static constexpr int BSC_fstatat = 0x040c0754;
static constexpr int BSC_fstatat64 = 0x040c0758;
static constexpr int BSC_linkat = 0x040c075c;
static constexpr int BSC_unlinkat = 0x040c0760;
static constexpr int BSC_readlinkat = 0x040c0764;
static constexpr int BSC_symlinkat = 0x040c0768;
static constexpr int BSC_mkdirat = 0x040c076c;
static constexpr int BSC_getattrlistat = 0x040c0770;

static constexpr int BSC_msync_extended = 0x040e0104;
static constexpr int BSC_pread_extended = 0x040e0264;
static constexpr int BSC_pwrite_extended = 0x040e0268;
static constexpr int BSC_mmap_extended = 0x040e0314;
static constexpr int BSC_mmap_extended2 = 0x040f0314;

// Carbon File Manager support
static constexpr int FILEMGR_PBGETCATALOGINFO = 0x1e000020;
static constexpr int FILEMGR_PBGETCATALOGINFOBULK = 0x1e000024;
static constexpr int FILEMGR_PBCREATEFILEUNICODE = 0x1e000028;
static constexpr int FILEMGR_PBCREATEDIRECTORYUNICODE = 0x1e00002c;
static constexpr int FILEMGR_PBCREATEFORK = 0x1e000030;
static constexpr int FILEMGR_PBDELETEFORK = 0x1e000034;
static constexpr int FILEMGR_PBITERATEFORK = 0x1e000038;
static constexpr int FILEMGR_PBOPENFORK = 0x1e00003c;
static constexpr int FILEMGR_PBREADFORK = 0x1e000040;
static constexpr int FILEMGR_PBWRITEFORK = 0x1e000044;
static constexpr int FILEMGR_PBALLOCATEFORK = 0x1e000048;
static constexpr int FILEMGR_PBDELETEOBJECT = 0x1e00004c;
static constexpr int FILEMGR_PBEXCHANGEOBJECT = 0x1e000050;
static constexpr int FILEMGR_PBGETFORKCBINFO = 0x1e000054;
static constexpr int FILEMGR_PBGETVOLUMEINFO = 0x1e000058;
static constexpr int FILEMGR_PBMAKEFSREF = 0x1e00005c;
static constexpr int FILEMGR_PBMAKEFSREFUNICODE = 0x1e000060;
static constexpr int FILEMGR_PBMOVEOBJECT = 0x1e000064;
static constexpr int FILEMGR_PBOPENITERATOR = 0x1e000068;
static constexpr int FILEMGR_PBRENAMEUNICODE = 0x1e00006c;
static constexpr int FILEMGR_PBSETCATALOGINFO = 0x1e000070;
static constexpr int FILEMGR_PBSETVOLUMEINFO = 0x1e000074;
static constexpr int FILEMGR_FSREFMAKEPATH = 0x1e000078;
static constexpr int FILEMGR_FSPATHMAKEREF = 0x1e00007c;

static constexpr int FILEMGR_PBGETCATINFO = 0x1e010000;
static constexpr int FILEMGR_PBGETCATINFOLITE = 0x1e010004;
static constexpr int FILEMGR_PBHGETFINFO = 0x1e010008;
static constexpr int FILEMGR_PBXGETVOLINFO = 0x1e01000c;
static constexpr int FILEMGR_PBHCREATE = 0x1e010010;
static constexpr int FILEMGR_PBHOPENDF = 0x1e010014;
static constexpr int FILEMGR_PBHOPENRF = 0x1e010018;
static constexpr int FILEMGR_PBHGETDIRACCESS = 0x1e01001c;
static constexpr int FILEMGR_PBHSETDIRACCESS = 0x1e010020;
static constexpr int FILEMGR_PBHMAPID = 0x1e010024;
static constexpr int FILEMGR_PBHMAPNAME = 0x1e010028;
static constexpr int FILEMGR_PBCLOSE = 0x1e01002c;
static constexpr int FILEMGR_PBFLUSHFILE = 0x1e010030;
static constexpr int FILEMGR_PBGETEOF = 0x1e010034;
static constexpr int FILEMGR_PBSETEOF = 0x1e010038;
static constexpr int FILEMGR_PBGETFPOS = 0x1e01003c;
static constexpr int FILEMGR_PBREAD = 0x1e010040;
static constexpr int FILEMGR_PBWRITE = 0x1e010044;
static constexpr int FILEMGR_PBGETFCBINFO = 0x1e010048;
static constexpr int FILEMGR_PBSETFINFO = 0x1e01004c;
static constexpr int FILEMGR_PBALLOCATE = 0x1e010050;
static constexpr int FILEMGR_PBALLOCCONTIG = 0x1e010054;
static constexpr int FILEMGR_PBSETFPOS = 0x1e010058;
static constexpr int FILEMGR_PBSETCATINFO = 0x1e01005c;
static constexpr int FILEMGR_PBGETVOLPARMS = 0x1e010060;
static constexpr int FILEMGR_PBSETVINFO = 0x1e010064;
static constexpr int FILEMGR_PBMAKEFSSPEC = 0x1e010068;
static constexpr int FILEMGR_PBHGETVINFO = 0x1e01006c;
static constexpr int FILEMGR_PBCREATEFILEIDREF = 0x1e010070;
static constexpr int FILEMGR_PBDELETEFILEIDREF = 0x1e010074;
static constexpr int FILEMGR_PBRESOLVEFILEIDREF = 0x1e010078;
static constexpr int FILEMGR_PBFLUSHVOL = 0x1e01007c;
static constexpr int FILEMGR_PBHRENAME = 0x1e010080;
static constexpr int FILEMGR_PBCATMOVE = 0x1e010084;
static constexpr int FILEMGR_PBEXCHANGEFILES = 0x1e010088;
static constexpr int FILEMGR_PBHDELETE = 0x1e01008c;
static constexpr int FILEMGR_PBDIRCREATE = 0x1e010090;
static constexpr int FILEMGR_PBCATSEARCH = 0x1e010094;
static constexpr int FILEMGR_PBHSETFLOCK = 0x1e010098;
static constexpr int FILEMGR_PBHRSTFLOCK = 0x1e01009c;
static constexpr int FILEMGR_PBLOCKRANGE = 0x1e0100a0;
static constexpr int FILEMGR_PBUNLOCKRANGE = 0x1e0100a4;


static constexpr int FILEMGR_CLASS = 0x1e;
static constexpr int FILEMGR_BASE = 0x1e000000;
