// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#include "SeccompSandbox.hpp"

#include <QDebug>

#ifdef __linux__
#include <errno.h>
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

// Macro helper BPF
#define ALLOW_SYSCALL(nr) \
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (nr), 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)

#define DENY_ALL BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS)

static sock_filter buildFilter[] = {
    /* Carica arch */ BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),
    /* Verifica x86-64 */
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
    /* Carica nr syscall */
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
    // ── Allow-list essenziale Qt6/KF6 ──────────────────────────────────────
    ALLOW_SYSCALL(__NR_read),
    ALLOW_SYSCALL(__NR_write),
    ALLOW_SYSCALL(__NR_open),
    ALLOW_SYSCALL(__NR_openat),
    ALLOW_SYSCALL(__NR_close),
    ALLOW_SYSCALL(__NR_stat),
    ALLOW_SYSCALL(__NR_fstat),
    ALLOW_SYSCALL(__NR_lstat),
    ALLOW_SYSCALL(__NR_newfstatat),
    ALLOW_SYSCALL(__NR_mmap),
    ALLOW_SYSCALL(__NR_mprotect),
    ALLOW_SYSCALL(__NR_munmap),
    ALLOW_SYSCALL(__NR_brk),
    ALLOW_SYSCALL(__NR_rt_sigaction),
    ALLOW_SYSCALL(__NR_rt_sigprocmask),
    ALLOW_SYSCALL(__NR_rt_sigreturn),
    ALLOW_SYSCALL(__NR_ioctl),
    ALLOW_SYSCALL(__NR_pread64),
    ALLOW_SYSCALL(__NR_pwrite64),
    ALLOW_SYSCALL(__NR_readv),
    ALLOW_SYSCALL(__NR_writev),
    ALLOW_SYSCALL(__NR_pipe),
    ALLOW_SYSCALL(__NR_pipe2),
    ALLOW_SYSCALL(__NR_select),
    ALLOW_SYSCALL(__NR_poll),
    ALLOW_SYSCALL(__NR_epoll_create1),
    ALLOW_SYSCALL(__NR_epoll_ctl),
    ALLOW_SYSCALL(__NR_epoll_wait),
    ALLOW_SYSCALL(__NR_epoll_pwait),
    ALLOW_SYSCALL(__NR_dup),
    ALLOW_SYSCALL(__NR_dup2),
    ALLOW_SYSCALL(__NR_dup3),
    ALLOW_SYSCALL(__NR_nanosleep),
    ALLOW_SYSCALL(__NR_getdents64),
    ALLOW_SYSCALL(__NR_getcwd),
    ALLOW_SYSCALL(__NR_chdir),
    ALLOW_SYSCALL(__NR_rename),
    ALLOW_SYSCALL(__NR_mkdir),
    ALLOW_SYSCALL(__NR_rmdir),
    ALLOW_SYSCALL(__NR_unlink),
    ALLOW_SYSCALL(__NR_symlink),
    ALLOW_SYSCALL(__NR_readlink),
    ALLOW_SYSCALL(__NR_chmod),
    ALLOW_SYSCALL(__NR_getuid),
    ALLOW_SYSCALL(__NR_geteuid),
    ALLOW_SYSCALL(__NR_getgid),
    ALLOW_SYSCALL(__NR_getegid),
    ALLOW_SYSCALL(__NR_getpid),
    ALLOW_SYSCALL(__NR_getppid),
    ALLOW_SYSCALL(__NR_getpgrp),
    ALLOW_SYSCALL(__NR_socket),
    ALLOW_SYSCALL(__NR_connect),
    ALLOW_SYSCALL(__NR_sendto),
    ALLOW_SYSCALL(__NR_recvfrom),
    ALLOW_SYSCALL(__NR_recvmsg),
    ALLOW_SYSCALL(__NR_sendmsg),
    ALLOW_SYSCALL(__NR_getsockname),
    ALLOW_SYSCALL(__NR_setsockopt),
    ALLOW_SYSCALL(__NR_getsockopt),
    ALLOW_SYSCALL(__NR_clone),
    ALLOW_SYSCALL(__NR_clone3),
    ALLOW_SYSCALL(__NR_fork),
    ALLOW_SYSCALL(__NR_vfork),
    ALLOW_SYSCALL(__NR_execve),
    ALLOW_SYSCALL(__NR_execveat),
    ALLOW_SYSCALL(__NR_wait4),
    ALLOW_SYSCALL(__NR_waitid),
    ALLOW_SYSCALL(__NR_exit),
    ALLOW_SYSCALL(__NR_exit_group),
    ALLOW_SYSCALL(__NR_futex),
    ALLOW_SYSCALL(__NR_set_robust_list),
    ALLOW_SYSCALL(__NR_get_robust_list),
    ALLOW_SYSCALL(__NR_madvise),
    ALLOW_SYSCALL(__NR_getrandom),
    ALLOW_SYSCALL(__NR_clock_gettime),
    ALLOW_SYSCALL(__NR_clock_nanosleep),
    ALLOW_SYSCALL(__NR_gettimeofday),
    ALLOW_SYSCALL(__NR_prctl),
    ALLOW_SYSCALL(__NR_arch_prctl),
    ALLOW_SYSCALL(__NR_seccomp),
    // ── D-Bus (socket Unix) ─────────────────────────────────────────────────
    ALLOW_SYSCALL(__NR_bind),
    ALLOW_SYSCALL(__NR_listen),
    ALLOW_SYSCALL(__NR_accept),
    ALLOW_SYSCALL(__NR_accept4),
    // ── Deny everything else ────────────────────────────────────────────────
    DENY_ALL,
};

#endif // __linux__

namespace PandaManager {

bool SeccompSandbox::isSupported() noexcept
{
#ifdef __linux__
    // Tenta un prctl no-op per verificare il supporto
    int ret = ::prctl(PR_GET_SECCOMP, 0, 0, 0, 0);
    return (ret >= 0 || errno != EINVAL);
#else
    return false;
#endif
}

bool SeccompSandbox::install() noexcept
{
#ifdef __linux__
    if (!isSupported()) {
        qWarning() << "[SeccompSandbox] seccomp non supportato dal kernel";
        return false;
    }

    // No new privs obbligatorio prima di seccomp
    if (::prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        qWarning() << "[SeccompSandbox] prctl(PR_SET_NO_NEW_PRIVS) fallito:" << errno;
        return false;
    }

    struct sock_fprog prog = {
        .len = static_cast<unsigned short>(sizeof(buildFilter) / sizeof(buildFilter[0])),
        .filter = buildFilter,
    };

    if (::prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) != 0) {
        qWarning() << "[SeccompSandbox] Installazione filtro seccomp fallita:" << errno;
        return false;
    }

    qInfo() << "[SeccompSandbox] Filtro seccomp-bpf installato con successo";
    return true;
#else
    qWarning() << "[SeccompSandbox] seccomp disponibile solo su Linux";
    return false;
#endif
}

} // namespace PandaManager
