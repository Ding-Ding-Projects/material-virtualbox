; $Id: SUPDrvA-win.asm 114348 2026-06-12 15:03:49Z knut.osmundsen@oracle.com $
;; @file
; VirtualBox Support Driver - Windows NT specific assembly parts.
;

;
; Copyright (C) 2006-2026 Oracle and/or its affiliates.
;
; This file is part of VirtualBox base platform packages, as
; available from https://www.virtualbox.org.
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation, in version 3 of the
; License.
;
; This program is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, see <https://www.gnu.org/licenses>.
;
; The contents of this file may alternatively be used under the terms
; of the Common Development and Distribution License Version 1.0
; (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
; in the VirtualBox distribution, in which case the provisions of the
; CDDL are applicable instead of those of the GPL.
;
; You may elect to license modified versions of this file under the
; terms and conditions of either the GPL or the CDDL or both.
;
; SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
;

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "iprt/asmdefs.mac"

BEGINCODE

%ifdef VBOX_WITH_HARDENING

 %ifdef RT_ARCH_X86
;
; Faking up ZwQueryVirtualMemory on XP and W2K3 where it's not exported.
; Using ZwOpenFile as a helper as it has the name number of parameters.
;
extern  IMPNAME(ZwOpenFile@24)

BEGINPROC supdrvNtQueryVirtualMemory_Xxx
  %macro NtQueryVirtualMemorySyscall 1
  GLOBALNAME supdrvNtQueryVirtualMemory_ %+ %1
        mov     eax, %1
        jmp     supdrvNtQueryVirtualMemory_Jump
  %endm
    NtQueryVirtualMemorySyscall 0xAF
    NtQueryVirtualMemorySyscall 0xB0
    NtQueryVirtualMemorySyscall 0xB1
    NtQueryVirtualMemorySyscall 0xB2
    NtQueryVirtualMemorySyscall 0xB3
    NtQueryVirtualMemorySyscall 0xB4
    NtQueryVirtualMemorySyscall 0xB5
    NtQueryVirtualMemorySyscall 0xB6
    NtQueryVirtualMemorySyscall 0xB7
    NtQueryVirtualMemorySyscall 0xB8
    NtQueryVirtualMemorySyscall 0xB9
    NtQueryVirtualMemorySyscall 0xBA
    NtQueryVirtualMemorySyscall 0xBB
    NtQueryVirtualMemorySyscall 0xBC
    NtQueryVirtualMemorySyscall 0xBD
    NtQueryVirtualMemorySyscall 0xBE

supdrvNtQueryVirtualMemory_Jump:
        mov     edx, IMP2(ZwOpenFile@24)
        lea     edx, [edx + 5]
        jmp     edx
ENDPROC   supdrvNtQueryVirtualMemory_Xxx

 %endif

 %ifdef RT_ARCH_AMD64
;
; Faking up ZwQueryVirtualMemory on XP64 and W2K3-64 where it's not exported.
; The C code locates and verifies the essentials in ZwRequestWaitReplyPort.
;
extern NAME(g_pfnKiServiceLinkage)
extern NAME(g_pfnKiServiceInternal)
BEGINPROC supdrvNtQueryVirtualMemory_Xxx
  %macro NtQueryVirtualMemorySyscall 1
  GLOBALNAME supdrvNtQueryVirtualMemory_ %+ %1
        mov     eax, %1
        jmp     supdrvNtQueryVirtualMemory_Jump
  %endm

    NtQueryVirtualMemorySyscall 0x1F
    NtQueryVirtualMemorySyscall 0x20
    NtQueryVirtualMemorySyscall 0x21
    NtQueryVirtualMemorySyscall 0x22
    NtQueryVirtualMemorySyscall 0x23

supdrvNtQueryVirtualMemory_Jump:
        cli
        mov     r10, rsp                ; save call frame pointer.
        mov     r11, [RT_WRT_RIP(NAME(g_pfnKiServiceLinkage))]
        push    0
        push    0
        push    r10                     ; call frame pointer (incoming rsp).
        pushfq
        push    10h
        push    r11                     ; r11 = KiServiceLinkage (ret w/ unwind info)
        jmp     qword [RT_WRT_RIP(NAME(g_pfnKiServiceInternal))]
ENDPROC   supdrvNtQueryVirtualMemory_Xxx
 %endif

%endif ; VBOX_WITH_HARDENING


%ifndef VBOX_WITH_HARDENING
;;
; Dummy byte so the .text section is not completely empty when hardening is disabled.
;
; With VBOX_WITH_HARDENING undefined, the whole body of this file above compiles out, leaving
; BEGINCODE's "section .text" with no content whatsoever. That is harmless for every assembler
; and every debug format except the one this driver's build template hands nasm unconditionally
; on Windows: CodeView 8 (-F cv8, see TEMPLATE_VBoxR0Drv_ASFLAGS.win.* / VBOX_NASM_ASFLAGS.pe.*
; in Config.kmk). Nasm's CodeView-8 writer never gets a single source-line record to attach to
; the file and asserts instead of just emitting an empty table:
;   panic: SUPDrvA-win.asm: assertion cv8_state.source_files != NULL failed at output/codeview.c:515
; This was verified against nasm 2.16.01 and 2.16.03 alike, so it is not a nasm version
; regression - upgrading nasm does not help. A per-source kBuild ASFLAGS override to drop -F cv8
; for just this file was investigated and rejected: kBuild's per-source FLAGS/ASFLAGS properties
; only ever *add* to the template-level flags (see kbuild_collect_source_prop() in kBuild's own
; kmk sources), they cannot subtract the -F cv8 that TEMPLATE_VBoxR0Drv_ASFLAGS already bakes in
; for every source in this target, and nasm has no "-F none" to cancel a format once given.
;
; A single unreferenced, unexported byte gives the CodeView writer the source-file record it
; wants. It changes nothing about the driver: nothing calls it, nothing exports it, and the
; hardened build (which always has real code above) never assembles this branch at all - the
; preprocessed output of this file is byte-for-byte identical with and without this block
; whenever VBOX_WITH_HARDENING is defined.
db 0
%endif ; !VBOX_WITH_HARDENING

