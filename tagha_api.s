	.file	"tagha_api.c"
	.intel_syntax noprefix
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"stdin"
.LC1:
	.string	"stderr"
.LC2:
	.string	"stdout"
	.text
	.p2align 4,,15
	.globl	Tagha_Init
	.type	Tagha_Init, @function
Tagha_Init:
.LFB49:
	.cfi_startproc
	push	ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	push	edi
	.cfi_def_cfa_offset 12
	.cfi_offset 7, -12
	push	esi
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	push	ebx
	.cfi_def_cfa_offset 20
	.cfi_offset 3, -20
	sub	esp, 28
	.cfi_def_cfa_offset 48
	mov	edx, DWORD PTR [esp+48]
	mov	ebx, DWORD PTR [esp+52]
	test	edx, edx
	je	.L1
	xor	eax, eax
	mov	ecx, 54
	mov	edi, edx
	test	ebx, ebx
	rep stosd
	mov	DWORD PTR [edx+200], ebx
	je	.L1
	mov	esi, DWORD PTR [ebx+7]
	lea	edx, [ebx+11+esi]
	mov	ebx, DWORD PTR [edx]
	lea	edi, [edx+4]
	mov	DWORD PTR [esp+16], edi
	test	ebx, ebx
	mov	DWORD PTR [esp+4], ebx
	je	.L1
	mov	esi, DWORD PTR [edi+5]
	lea	ecx, [ebx-1]
	add	edx, 13
	mov	eax, DWORD PTR [edi+1]
	mov	edi, edx
	and	ecx, 3
	mov	DWORD PTR [esp+12], ecx
	mov	DWORD PTR [esp+8], esi
	mov	ecx, 6
	mov	esi, OFFSET FLAT:.LC0
	repz cmpsb
	seta	bl
	setb	cl
	cmp	bl, cl
	je	.L94
	add	eax, DWORD PTR [esp+8]
	cmp	DWORD PTR [esp+4], 1
	mov	ebp, 1
	lea	edi, [edx+eax]
	je	.L7
	mov	edx, DWORD PTR [esp+12]
	test	edx, edx
	je	.L106
	cmp	edx, 1
	je	.L81
	cmp	edx, 2
	je	.L82
	lea	edx, [edi+9]
	mov	eax, DWORD PTR [edi+1]
	mov	ebx, DWORD PTR [edi+5]
	mov	esi, OFFSET FLAT:.LC0
	mov	ecx, 6
	mov	ebp, 2
	mov	edi, edx
	repz cmpsb
	seta	cl
	mov	esi, ecx
	setb	cl
	add	ebx, eax
	lea	edi, [edx+ebx]
	mov	ebx, esi
	cmp	bl, cl
	je	.L94
.L82:
	lea	edx, [edi+9]
	mov	eax, DWORD PTR [edi+1]
	mov	ebx, DWORD PTR [edi+5]
	mov	ecx, 6
	mov	esi, OFFSET FLAT:.LC0
	mov	edi, edx
	repz cmpsb
	seta	cl
	mov	esi, ecx
	setb	cl
	add	ebx, eax
	lea	edi, [edx+ebx]
	mov	ebx, esi
	add	ebp, 1
	cmp	bl, cl
	je	.L94
.L81:
	lea	edx, [edi+9]
	mov	eax, DWORD PTR [edi+1]
	mov	edi, DWORD PTR [edi+5]
	mov	ecx, 6
	mov	esi, OFFSET FLAT:.LC0
	mov	DWORD PTR [esp+8], edi
	mov	edi, edx
	repz cmpsb
	seta	bl
	setb	cl
	cmp	bl, cl
	je	.L94
	add	eax, DWORD PTR [esp+8]
	add	ebp, 1
	cmp	DWORD PTR [esp+4], ebp
	lea	edi, [edx+eax]
	je	.L7
.L106:
	mov	DWORD PTR [esp+8], ebp
.L8:
	mov	ebp, DWORD PTR [edi+5]
	lea	edx, [edi+9]
	mov	ebx, DWORD PTR [edi+1]
	mov	esi, OFFSET FLAT:.LC0
	mov	edi, edx
	mov	DWORD PTR [esp+12], ebp
	mov	ebp, 6
	mov	ecx, ebp
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L108
	mov	esi, DWORD PTR [esp+12]
	mov	ecx, ebp
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC0
	mov	DWORD PTR [esp+12], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L108
	mov	esi, DWORD PTR [esp+12]
	mov	ecx, ebp
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC0
	mov	DWORD PTR [esp+12], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L108
	mov	esi, DWORD PTR [esp+12]
	mov	ecx, ebp
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC0
	mov	DWORD PTR [esp+12], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L108
	mov	esi, DWORD PTR [esp+12]
	add	DWORD PTR [esp+8], 4
	mov	ebp, DWORD PTR [esp+8]
	add	esi, ebx
	cmp	DWORD PTR [esp+4], ebp
	lea	edi, [edx+esi]
	jne	.L8
	.p2align 4,,10
	.p2align 3
.L7:
	mov	ecx, DWORD PTR [esp+4]
	mov	ebp, DWORD PTR [esp+16]
	lea	eax, [ecx-1]
	mov	esi, DWORD PTR [ebp+1]
	lea	edx, [ebp+9]
	and	eax, 3
	mov	edi, edx
	mov	DWORD PTR [esp+20], eax
	mov	eax, DWORD PTR [ebp+5]
	mov	ebp, 7
	mov	ecx, ebp
	mov	DWORD PTR [esp+12], esi
	mov	esi, OFFSET FLAT:.LC1
	repz cmpsb
	seta	bl
	setb	cl
	cmp	bl, cl
	je	.L95
	add	eax, DWORD PTR [esp+12]
	cmp	DWORD PTR [esp+4], 1
	mov	DWORD PTR [esp+8], 1
	lea	ecx, [edx+eax]
	je	.L11
	mov	edx, DWORD PTR [esp+20]
	test	edx, edx
	je	.L12
	cmp	edx, 1
	je	.L78
	cmp	edx, 2
	je	.L79
	lea	edx, [ecx+9]
	mov	eax, DWORD PTR [ecx+1]
	mov	ebx, DWORD PTR [ecx+5]
	mov	esi, OFFSET FLAT:.LC1
	mov	ecx, ebp
	mov	DWORD PTR [esp+8], 2
	mov	edi, edx
	repz cmpsb
	mov	DWORD PTR [esp+12], eax
	seta	cl
	mov	ebp, ecx
	setb	cl
	add	ebx, eax
	mov	esi, ecx
	lea	ecx, [edx+ebx]
	mov	ebx, ebp
	mov	eax, esi
	cmp	bl, al
	je	.L95
.L79:
	lea	edx, [ecx+9]
	mov	ebp, DWORD PTR [ecx+1]
	mov	ebx, DWORD PTR [ecx+5]
	mov	esi, OFFSET FLAT:.LC1
	mov	ecx, 7
	mov	edi, edx
	repz cmpsb
	mov	DWORD PTR [esp+12], ebp
	setb	cl
	seta	al
	add	ebx, ebp
	mov	esi, ecx
	lea	ecx, [edx+ebx]
	add	DWORD PTR [esp+8], 1
	mov	ebx, esi
	cmp	al, bl
	je	.L95
.L78:
	mov	ebp, DWORD PTR [ecx+1]
	lea	edx, [ecx+9]
	mov	esi, OFFSET FLAT:.LC1
	mov	ebx, DWORD PTR [ecx+5]
	mov	ecx, 7
	mov	edi, edx
	mov	DWORD PTR [esp+12], ebp
	repz cmpsb
	je	.L95
	add	DWORD PTR [esp+8], 1
	add	ebx, ebp
	mov	eax, DWORD PTR [esp+8]
	cmp	DWORD PTR [esp+4], eax
	lea	ecx, [edx+ebx]
	je	.L11
.L12:
	lea	edx, [ecx+9]
	mov	ebx, DWORD PTR [ecx+1]
	mov	ecx, DWORD PTR [ecx+5]
	mov	ebp, 7
	mov	esi, OFFSET FLAT:.LC1
	mov	edi, edx
	mov	DWORD PTR [esp+12], ecx
	mov	ecx, ebp
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L104
	mov	esi, DWORD PTR [esp+12]
	mov	ecx, ebp
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC1
	mov	DWORD PTR [esp+12], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L104
	mov	esi, DWORD PTR [esp+12]
	mov	ecx, ebp
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC1
	mov	DWORD PTR [esp+12], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L104
	mov	esi, DWORD PTR [esp+12]
	mov	ecx, ebp
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC1
	mov	DWORD PTR [esp+12], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L104
	mov	esi, DWORD PTR [esp+12]
	add	DWORD PTR [esp+8], 4
	mov	ebp, DWORD PTR [esp+8]
	add	esi, ebx
	cmp	DWORD PTR [esp+4], ebp
	lea	ecx, [edx+esi]
	jne	.L12
	.p2align 4,,10
	.p2align 3
.L11:
	mov	eax, DWORD PTR [esp+4]
	mov	esi, DWORD PTR [esp+16]
	lea	ebp, [eax-1]
	mov	ecx, DWORD PTR [esi+1]
	lea	edx, [esi+9]
	mov	eax, DWORD PTR [esi+5]
	mov	esi, OFFSET FLAT:.LC2
	and	ebp, 3
	mov	edi, edx
	mov	DWORD PTR [esp+20], ebp
	mov	ebp, 7
	mov	DWORD PTR [esp+12], ecx
	mov	ecx, ebp
	repz cmpsb
	seta	bl
	setb	cl
	cmp	bl, cl
	je	.L96
	add	eax, DWORD PTR [esp+12]
	cmp	DWORD PTR [esp+4], 1
	mov	DWORD PTR [esp+8], 1
	lea	ecx, [edx+eax]
	je	.L1
	mov	edx, DWORD PTR [esp+20]
	test	edx, edx
	je	.L15
	cmp	edx, 1
	je	.L75
	cmp	edx, 2
	je	.L76
	lea	edx, [ecx+9]
	mov	eax, DWORD PTR [ecx+1]
	mov	ebx, DWORD PTR [ecx+5]
	mov	esi, OFFSET FLAT:.LC2
	mov	ecx, ebp
	mov	DWORD PTR [esp+8], 2
	mov	edi, edx
	repz cmpsb
	mov	DWORD PTR [esp+12], eax
	seta	cl
	mov	ebp, ecx
	setb	cl
	add	ebx, eax
	mov	esi, ecx
	lea	ecx, [edx+ebx]
	mov	ebx, ebp
	mov	eax, esi
	cmp	bl, al
	je	.L96
.L76:
	lea	edx, [ecx+9]
	mov	ebp, DWORD PTR [ecx+1]
	mov	ebx, DWORD PTR [ecx+5]
	mov	esi, OFFSET FLAT:.LC2
	mov	ecx, 7
	mov	edi, edx
	repz cmpsb
	mov	DWORD PTR [esp+12], ebp
	setb	cl
	seta	al
	add	ebx, ebp
	mov	esi, ecx
	lea	ecx, [edx+ebx]
	add	DWORD PTR [esp+8], 1
	mov	ebx, esi
	cmp	al, bl
	je	.L96
.L75:
	mov	ebp, DWORD PTR [ecx+1]
	lea	edx, [ecx+9]
	mov	esi, OFFSET FLAT:.LC2
	mov	ebx, DWORD PTR [ecx+5]
	mov	ecx, 7
	mov	edi, edx
	mov	DWORD PTR [esp+12], ebp
	repz cmpsb
	je	.L96
	add	DWORD PTR [esp+8], 1
	add	ebx, ebp
	mov	eax, DWORD PTR [esp+8]
	cmp	DWORD PTR [esp+4], eax
	lea	ecx, [edx+ebx]
	je	.L1
.L15:
	lea	edx, [ecx+9]
	mov	ebx, DWORD PTR [ecx+1]
	mov	ecx, DWORD PTR [ecx+5]
	mov	ebp, 7
	mov	esi, OFFSET FLAT:.LC2
	mov	edi, edx
	mov	DWORD PTR [esp+12], ecx
	mov	ecx, ebp
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L100
	mov	esi, DWORD PTR [esp+12]
	mov	ecx, ebp
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC2
	mov	DWORD PTR [esp+12], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L100
	mov	esi, DWORD PTR [esp+12]
	mov	ecx, ebp
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC2
	mov	DWORD PTR [esp+12], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L100
	mov	esi, DWORD PTR [esp+12]
	mov	ecx, ebp
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC2
	mov	DWORD PTR [esp+12], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L100
	mov	esi, DWORD PTR [esp+12]
	add	DWORD PTR [esp+8], 4
	mov	ebp, DWORD PTR [esp+8]
	add	esi, ebx
	cmp	DWORD PTR [esp+4], ebp
	lea	ecx, [edx+esi]
	jne	.L15
.L1:
	add	esp, 28
	.cfi_remember_state
	.cfi_def_cfa_offset 20
	pop	ebx
	.cfi_restore 3
	.cfi_def_cfa_offset 16
	pop	esi
	.cfi_restore 6
	.cfi_def_cfa_offset 12
	pop	edi
	.cfi_restore 7
	.cfi_def_cfa_offset 8
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa_offset 4
	ret
	.p2align 4,,10
	.p2align 3
.L108:
	.cfi_restore_state
	mov	eax, ebx
.L94:
	add	eax, edx
	je	.L7
	mov	ebx, DWORD PTR stdin
	mov	DWORD PTR [eax], ebx
	jmp	.L7
	.p2align 4,,10
	.p2align 3
.L100:
	mov	DWORD PTR [esp+12], ebx
.L96:
	mov	ebx, DWORD PTR [esp+12]
	add	ebx, edx
	je	.L1
	mov	edi, DWORD PTR stdout
	mov	DWORD PTR [ebx], edi
	add	esp, 28
	.cfi_remember_state
	.cfi_def_cfa_offset 20
	pop	ebx
	.cfi_restore 3
	.cfi_def_cfa_offset 16
	pop	esi
	.cfi_restore 6
	.cfi_def_cfa_offset 12
	pop	edi
	.cfi_restore 7
	.cfi_def_cfa_offset 8
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa_offset 4
	ret
	.p2align 4,,10
	.p2align 3
.L104:
	.cfi_restore_state
	mov	DWORD PTR [esp+12], ebx
.L95:
	mov	ebx, DWORD PTR [esp+12]
	add	ebx, edx
	je	.L11
	mov	edi, DWORD PTR stderr
	mov	DWORD PTR [ebx], edi
	jmp	.L11
	.cfi_endproc
.LFE49:
	.size	Tagha_Init, .-Tagha_Init
	.p2align 4,,15
	.globl	Tagha_InitN
	.type	Tagha_InitN, @function
Tagha_InitN:
.LFB50:
	.cfi_startproc
	push	ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	push	edi
	.cfi_def_cfa_offset 12
	.cfi_offset 7, -12
	push	esi
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	push	ebx
	.cfi_def_cfa_offset 20
	.cfi_offset 3, -20
	sub	esp, 28
	.cfi_def_cfa_offset 48
	mov	edx, DWORD PTR [esp+48]
	test	edx, edx
	je	.L111
	mov	ebx, DWORD PTR [esp+52]
	xor	eax, eax
	mov	ecx, 54
	mov	edi, edx
	rep stosd
	test	ebx, ebx
	mov	DWORD PTR [edx+200], ebx
	je	.L114
	mov	esi, DWORD PTR [ebx+7]
	mov	ebp, DWORD PTR [esp+52]
	lea	ebx, [ebp+11+esi]
	mov	edx, DWORD PTR [ebx]
	lea	ecx, [ebx+4]
	mov	DWORD PTR [esp+8], ecx
	test	edx, edx
	mov	DWORD PTR [esp], edx
	je	.L114
	add	ebx, 13
	mov	eax, DWORD PTR [ecx+5]
	sub	edx, 1
	mov	ebp, DWORD PTR [ecx+1]
	mov	esi, OFFSET FLAT:.LC0
	mov	ecx, 6
	mov	edi, ebx
	and	edx, 3
	repz cmpsb
	mov	DWORD PTR [esp+12], eax
	setb	cl
	seta	al
	cmp	al, cl
	je	.L251
	mov	eax, DWORD PTR [esp+12]
	mov	DWORD PTR [esp+4], 1
	add	eax, ebp
	add	eax, ebx
	cmp	DWORD PTR [esp], 1
	je	.L118
	test	edx, edx
	je	.L119
	cmp	edx, 1
	je	.L230
	cmp	edx, 2
	je	.L231
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	ecx, 6
	mov	eax, DWORD PTR [eax+5]
	mov	esi, OFFSET FLAT:.LC0
	mov	DWORD PTR [esp+4], 2
	mov	edi, ebx
	repz cmpsb
	seta	cl
	setb	dl
	add	eax, ebp
	add	eax, ebx
	cmp	cl, dl
	je	.L251
.L231:
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	ecx, 6
	mov	eax, DWORD PTR [eax+5]
	mov	esi, OFFSET FLAT:.LC0
	mov	edi, ebx
	repz cmpsb
	seta	cl
	setb	dl
	add	eax, ebp
	add	DWORD PTR [esp+4], 1
	add	eax, ebx
	cmp	cl, dl
	je	.L251
.L230:
	lea	ebx, [eax+9]
	mov	ecx, 6
	mov	esi, OFFSET FLAT:.LC0
	mov	ebp, DWORD PTR [eax+1]
	mov	edi, ebx
	mov	eax, DWORD PTR [eax+5]
	repz cmpsb
	je	.L251
	add	DWORD PTR [esp+4], 1
	add	eax, ebp
	mov	edx, DWORD PTR [esp+4]
	add	eax, ebx
	cmp	DWORD PTR [esp], edx
	je	.L118
.L119:
	lea	ebx, [eax+9]
	mov	ecx, 6
	mov	esi, OFFSET FLAT:.LC0
	mov	ebp, DWORD PTR [eax+1]
	mov	eax, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	seta	dl
	setb	cl
	cmp	dl, cl
	je	.L251
	add	eax, ebp
	mov	esi, OFFSET FLAT:.LC0
	mov	ecx, 6
	add	eax, ebx
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	eax, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	je	.L251
	add	eax, ebp
	mov	esi, OFFSET FLAT:.LC0
	mov	ecx, 6
	add	eax, ebx
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	edx, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	je	.L251
	add	edx, ebp
	mov	esi, OFFSET FLAT:.LC0
	mov	ecx, 6
	add	edx, ebx
	lea	ebx, [edx+9]
	mov	ebp, DWORD PTR [edx+1]
	mov	eax, DWORD PTR [edx+5]
	mov	edi, ebx
	repz cmpsb
	je	.L251
	add	DWORD PTR [esp+4], 4
	add	eax, ebp
	mov	edx, DWORD PTR [esp+4]
	add	eax, ebx
	cmp	DWORD PTR [esp], edx
	jne	.L119
	.p2align 4,,10
	.p2align 3
.L118:
	mov	ebx, DWORD PTR [esp]
	mov	eax, DWORD PTR [esp+8]
	mov	ecx, 7
	mov	esi, OFFSET FLAT:.LC1
	sub	ebx, 1
	mov	ebp, DWORD PTR [eax+1]
	and	ebx, 3
	mov	DWORD PTR [esp+12], ebx
	lea	ebx, [eax+9]
	mov	eax, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	seta	dl
	setb	cl
	cmp	dl, cl
	je	.L252
	add	eax, ebp
	mov	DWORD PTR [esp+4], 1
	add	eax, ebx
	cmp	DWORD PTR [esp], 1
	je	.L122
	mov	ebp, DWORD PTR [esp+12]
	test	ebp, ebp
	je	.L123
	cmp	ebp, 1
	je	.L227
	cmp	ebp, 2
	je	.L228
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	ecx, 7
	mov	eax, DWORD PTR [eax+5]
	mov	esi, OFFSET FLAT:.LC1
	mov	DWORD PTR [esp+4], 2
	mov	edi, ebx
	repz cmpsb
	seta	cl
	setb	dl
	add	eax, ebp
	add	eax, ebx
	cmp	cl, dl
	je	.L252
.L228:
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	ecx, 7
	mov	eax, DWORD PTR [eax+5]
	mov	esi, OFFSET FLAT:.LC1
	mov	edi, ebx
	repz cmpsb
	seta	cl
	setb	dl
	add	eax, ebp
	add	DWORD PTR [esp+4], 1
	add	eax, ebx
	cmp	cl, dl
	je	.L252
.L227:
	lea	ebx, [eax+9]
	mov	ecx, 7
	mov	esi, OFFSET FLAT:.LC1
	mov	ebp, DWORD PTR [eax+1]
	mov	edi, ebx
	mov	eax, DWORD PTR [eax+5]
	repz cmpsb
	je	.L252
	add	DWORD PTR [esp+4], 1
	add	eax, ebp
	mov	edx, DWORD PTR [esp+4]
	add	eax, ebx
	cmp	DWORD PTR [esp], edx
	je	.L122
.L123:
	lea	ebx, [eax+9]
	mov	ecx, 7
	mov	esi, OFFSET FLAT:.LC1
	mov	ebp, DWORD PTR [eax+1]
	mov	eax, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	seta	dl
	setb	cl
	cmp	dl, cl
	je	.L252
	add	eax, ebp
	mov	esi, OFFSET FLAT:.LC1
	mov	ecx, 7
	add	eax, ebx
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	eax, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	je	.L252
	add	eax, ebp
	mov	esi, OFFSET FLAT:.LC1
	mov	ecx, 7
	add	eax, ebx
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	edx, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	je	.L252
	add	edx, ebp
	mov	esi, OFFSET FLAT:.LC1
	mov	ecx, 7
	add	edx, ebx
	lea	ebx, [edx+9]
	mov	ebp, DWORD PTR [edx+1]
	mov	eax, DWORD PTR [edx+5]
	mov	edi, ebx
	repz cmpsb
	je	.L252
	add	DWORD PTR [esp+4], 4
	add	eax, ebp
	mov	edx, DWORD PTR [esp+4]
	add	eax, ebx
	cmp	DWORD PTR [esp], edx
	jne	.L123
	.p2align 4,,10
	.p2align 3
.L122:
	mov	ebx, DWORD PTR [esp]
	mov	eax, DWORD PTR [esp+8]
	mov	ecx, 7
	mov	esi, OFFSET FLAT:.LC2
	sub	ebx, 1
	mov	ebp, DWORD PTR [eax+1]
	and	ebx, 3
	mov	DWORD PTR [esp+12], ebx
	lea	ebx, [eax+9]
	mov	eax, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	seta	dl
	setb	cl
	cmp	dl, cl
	je	.L253
	add	eax, ebp
	mov	DWORD PTR [esp+4], 1
	add	eax, ebx
	cmp	DWORD PTR [esp], 1
	je	.L114
	mov	ebp, DWORD PTR [esp+12]
	test	ebp, ebp
	je	.L126
	cmp	ebp, 1
	je	.L224
	cmp	ebp, 2
	je	.L225
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	ecx, 7
	mov	eax, DWORD PTR [eax+5]
	mov	esi, OFFSET FLAT:.LC2
	mov	DWORD PTR [esp+4], 2
	mov	edi, ebx
	repz cmpsb
	seta	cl
	setb	dl
	add	eax, ebp
	add	eax, ebx
	cmp	cl, dl
	je	.L253
.L225:
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	ecx, 7
	mov	eax, DWORD PTR [eax+5]
	mov	esi, OFFSET FLAT:.LC2
	mov	edi, ebx
	repz cmpsb
	seta	cl
	setb	dl
	add	eax, ebp
	add	DWORD PTR [esp+4], 1
	add	eax, ebx
	cmp	cl, dl
	je	.L253
.L224:
	lea	ebx, [eax+9]
	mov	ecx, 7
	mov	esi, OFFSET FLAT:.LC2
	mov	ebp, DWORD PTR [eax+1]
	mov	edi, ebx
	mov	eax, DWORD PTR [eax+5]
	repz cmpsb
	je	.L253
	add	DWORD PTR [esp+4], 1
	add	eax, ebp
	mov	edx, DWORD PTR [esp+4]
	add	eax, ebx
	cmp	DWORD PTR [esp], edx
	je	.L114
.L126:
	lea	ebx, [eax+9]
	mov	ecx, 7
	mov	esi, OFFSET FLAT:.LC2
	mov	ebp, DWORD PTR [eax+1]
	mov	eax, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	seta	dl
	setb	cl
	cmp	dl, cl
	je	.L253
	add	eax, ebp
	mov	esi, OFFSET FLAT:.LC2
	mov	ecx, 7
	add	eax, ebx
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	eax, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	je	.L253
	add	eax, ebp
	mov	esi, OFFSET FLAT:.LC2
	mov	ecx, 7
	add	eax, ebx
	lea	ebx, [eax+9]
	mov	ebp, DWORD PTR [eax+1]
	mov	edx, DWORD PTR [eax+5]
	mov	edi, ebx
	repz cmpsb
	je	.L253
	add	edx, ebp
	mov	esi, OFFSET FLAT:.LC2
	mov	ecx, 7
	add	edx, ebx
	lea	ebx, [edx+9]
	mov	ebp, DWORD PTR [edx+1]
	mov	eax, DWORD PTR [edx+5]
	mov	edi, ebx
	repz cmpsb
	je	.L253
	add	DWORD PTR [esp+4], 4
	add	eax, ebp
	mov	edx, DWORD PTR [esp+4]
	add	eax, ebx
	cmp	DWORD PTR [esp], edx
	jne	.L126
.L114:
	mov	ebx, DWORD PTR [esp+56]
	test	ebx, ebx
	je	.L111
	.p2align 4,,10
	.p2align 3
.L276:
	mov	ebp, DWORD PTR [esp+56]
	mov	eax, DWORD PTR [ebp+4]
	test	eax, eax
	mov	DWORD PTR [esp+8], eax
	je	.L111
	mov	esi, DWORD PTR [esp+56]
	mov	ebp, DWORD PTR [esi]
	test	ebp, ebp
	je	.L111
	mov	edi, DWORD PTR [esp+52]
	add	edi, 15
	mov	DWORD PTR [esp+12], edi
	.p2align 4,,10
	.p2align 3
.L132:
	mov	ecx, DWORD PTR [esp+52]
	test	ecx, ecx
	je	.L128
	mov	edx, DWORD PTR [esp+52]
	mov	ebx, DWORD PTR [edx+11]
	test	ebx, ebx
	mov	DWORD PTR [esp+4], ebx
	je	.L128
	mov	esi, DWORD PTR [esp+12]
	lea	eax, [ebx-1]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	and	eax, 3
	lea	edi, [esi+9]
	mov	DWORD PTR [esp+8], eax
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 60
	push	ebp
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	mov	ecx, DWORD PTR [esp]
	je	.L254
	add	esi, ebx
	mov	DWORD PTR [esp], 1
	add	esi, edi
	cmp	DWORD PTR [esp+4], 1
	je	.L128
	test	ecx, ecx
	je	.L131
	cmp	ecx, 1
	je	.L221
	cmp	ecx, 2
	je	.L222
	mov	ebx, DWORD PTR [esi+1]
	lea	edi, [esi+9]
	mov	esi, DWORD PTR [esi+5]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	push	edi
	.cfi_def_cfa_offset 60
	push	ebp
	.cfi_def_cfa_offset 64
	add	esi, ebx
	call	strcmp
	add	esi, edi
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	mov	DWORD PTR [esp], 2
	je	.L254
.L222:
	lea	edi, [esi+9]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 60
	push	ebp
	.cfi_def_cfa_offset 64
	add	esi, ebx
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	add	esi, edi
	add	DWORD PTR [esp], 1
	test	eax, eax
	je	.L254
.L221:
	lea	edi, [esi+9]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 60
	push	ebp
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L254
	add	DWORD PTR [esp], 1
	add	esi, ebx
	mov	edx, DWORD PTR [esp]
	add	esi, edi
	cmp	DWORD PTR [esp+4], edx
	je	.L128
.L131:
	lea	edi, [esi+9]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 60
	push	ebp
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L254
	add	esi, ebx
	sub	esp, 8
	.cfi_def_cfa_offset 56
	add	esi, edi
	lea	edi, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 60
	push	ebp
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L254
	add	esi, ebx
	sub	esp, 8
	.cfi_def_cfa_offset 56
	add	esi, edi
	lea	edi, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 60
	push	ebp
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L254
	add	esi, ebx
	sub	esp, 8
	.cfi_def_cfa_offset 56
	add	esi, edi
	lea	edi, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 60
	push	ebp
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L254
	add	DWORD PTR [esp], 4
	add	esi, ebx
	mov	edx, DWORD PTR [esp]
	add	esi, edi
	cmp	DWORD PTR [esp+4], edx
	jne	.L131
	.p2align 4,,10
	.p2align 3
.L128:
	add	DWORD PTR [esp+56], 8
	mov	eax, DWORD PTR [esp+56]
	mov	ecx, DWORD PTR [eax+4]
	test	ecx, ecx
	mov	DWORD PTR [esp+8], ecx
	je	.L111
	mov	edx, DWORD PTR [esp+56]
	mov	ebp, DWORD PTR [edx]
	test	ebp, ebp
	jne	.L132
.L111:
	add	esp, 28
	.cfi_remember_state
	.cfi_def_cfa_offset 20
	pop	ebx
	.cfi_restore 3
	.cfi_def_cfa_offset 16
	pop	esi
	.cfi_restore 6
	.cfi_def_cfa_offset 12
	pop	edi
	.cfi_restore 7
	.cfi_def_cfa_offset 8
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa_offset 4
	ret
	.p2align 4,,10
	.p2align 3
.L254:
	.cfi_restore_state
	add	ebx, edi
	je	.L128
	mov	ebp, DWORD PTR [esp+8]
	mov	DWORD PTR [ebx], ebp
	jmp	.L128
	.p2align 4,,10
	.p2align 3
.L251:
	add	ebx, ebp
	je	.L118
	mov	edx, DWORD PTR stdin
	mov	DWORD PTR [ebx], edx
	jmp	.L118
	.p2align 4,,10
	.p2align 3
.L253:
	add	ebx, ebp
	je	.L114
	mov	edx, DWORD PTR stdout
	mov	DWORD PTR [ebx], edx
	mov	ebx, DWORD PTR [esp+56]
	test	ebx, ebx
	jne	.L276
	jmp	.L111
	.p2align 4,,10
	.p2align 3
.L252:
	add	ebx, ebp
	je	.L122
	mov	edx, DWORD PTR stderr
	mov	DWORD PTR [ebx], edx
	jmp	.L122
	.cfi_endproc
.LFE50:
	.size	Tagha_InitN, .-Tagha_InitN
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align 4
.LC3:
	.ascii	"=== Tagha VM State Info ===\n\nPrinting Registers:\nregister"
	.ascii	" alaf: '%llu'\nregister beth: '%llu'\nregister gamal: '%llu'"
	.ascii	"\nregister dalath: '%llu'\nregister heh: '%llu'\nregister wa"
	.ascii	"w: '%llu'\nregister zain: '%llu'\nregister heth: '%llu'\nreg"
	.ascii	"ister teth: '%llu'\nregister yodh: '%llu'\nregister kaf: '%l"
	.ascii	"lu'\nregister lamadh: '%llu'\nregister meem: '%llu'\nregiste"
	.ascii	"r noon: '%llu'\nregister semkath: '%llu'\nregister eh: '%llu"
	.ascii	"'\nregister peh: '%llu'\nregister"
	.string	" sadhe: '%llu'\nregister qof: '%llu'\nregister reesh: '%llu'\nregister sheen: '%llu'\nregister taw: '%llu'\nregister stack pointer: '%p'\nregister base pointer: '%p'\nregister instruction pointer: '%p'\n\nPrinting Condition Flag: %u\n=== End Tagha VM State Info ===\n"
	.text
	.p2align 4,,15
	.globl	Tagha_PrintVMState
	.type	Tagha_PrintVMState, @function
Tagha_PrintVMState:
.LFB51:
	.cfi_startproc
	sub	esp, 12
	.cfi_def_cfa_offset 16
	mov	eax, DWORD PTR [esp+16]
	test	eax, eax
	je	.L277
	movzx	edx, BYTE PTR [eax+212]
	sub	esp, 8
	.cfi_def_cfa_offset 24
	and	edx, 1
	push	edx
	.cfi_def_cfa_offset 28
	push	DWORD PTR [eax+192]
	.cfi_def_cfa_offset 32
	push	DWORD PTR [eax+184]
	.cfi_def_cfa_offset 36
	push	DWORD PTR [eax+176]
	.cfi_def_cfa_offset 40
	push	DWORD PTR [eax+172]
	.cfi_def_cfa_offset 44
	push	DWORD PTR [eax+168]
	.cfi_def_cfa_offset 48
	push	DWORD PTR [eax+164]
	.cfi_def_cfa_offset 52
	push	DWORD PTR [eax+160]
	.cfi_def_cfa_offset 56
	push	DWORD PTR [eax+156]
	.cfi_def_cfa_offset 60
	push	DWORD PTR [eax+152]
	.cfi_def_cfa_offset 64
	push	DWORD PTR [eax+148]
	.cfi_def_cfa_offset 68
	push	DWORD PTR [eax+144]
	.cfi_def_cfa_offset 72
	push	DWORD PTR [eax+140]
	.cfi_def_cfa_offset 76
	push	DWORD PTR [eax+136]
	.cfi_def_cfa_offset 80
	push	DWORD PTR [eax+132]
	.cfi_def_cfa_offset 84
	push	DWORD PTR [eax+128]
	.cfi_def_cfa_offset 88
	push	DWORD PTR [eax+124]
	.cfi_def_cfa_offset 92
	push	DWORD PTR [eax+120]
	.cfi_def_cfa_offset 96
	push	DWORD PTR [eax+116]
	.cfi_def_cfa_offset 100
	push	DWORD PTR [eax+112]
	.cfi_def_cfa_offset 104
	push	DWORD PTR [eax+108]
	.cfi_def_cfa_offset 108
	push	DWORD PTR [eax+104]
	.cfi_def_cfa_offset 112
	push	DWORD PTR [eax+100]
	.cfi_def_cfa_offset 116
	push	DWORD PTR [eax+96]
	.cfi_def_cfa_offset 120
	push	DWORD PTR [eax+92]
	.cfi_def_cfa_offset 124
	push	DWORD PTR [eax+88]
	.cfi_def_cfa_offset 128
	push	DWORD PTR [eax+84]
	.cfi_def_cfa_offset 132
	push	DWORD PTR [eax+80]
	.cfi_def_cfa_offset 136
	push	DWORD PTR [eax+76]
	.cfi_def_cfa_offset 140
	push	DWORD PTR [eax+72]
	.cfi_def_cfa_offset 144
	push	DWORD PTR [eax+68]
	.cfi_def_cfa_offset 148
	push	DWORD PTR [eax+64]
	.cfi_def_cfa_offset 152
	push	DWORD PTR [eax+60]
	.cfi_def_cfa_offset 156
	push	DWORD PTR [eax+56]
	.cfi_def_cfa_offset 160
	push	DWORD PTR [eax+52]
	.cfi_def_cfa_offset 164
	push	DWORD PTR [eax+48]
	.cfi_def_cfa_offset 168
	push	DWORD PTR [eax+44]
	.cfi_def_cfa_offset 172
	push	DWORD PTR [eax+40]
	.cfi_def_cfa_offset 176
	push	DWORD PTR [eax+36]
	.cfi_def_cfa_offset 180
	push	DWORD PTR [eax+32]
	.cfi_def_cfa_offset 184
	push	DWORD PTR [eax+28]
	.cfi_def_cfa_offset 188
	push	DWORD PTR [eax+24]
	.cfi_def_cfa_offset 192
	push	DWORD PTR [eax+20]
	.cfi_def_cfa_offset 196
	push	DWORD PTR [eax+16]
	.cfi_def_cfa_offset 200
	push	DWORD PTR [eax+12]
	.cfi_def_cfa_offset 204
	push	DWORD PTR [eax+8]
	.cfi_def_cfa_offset 208
	push	DWORD PTR [eax+4]
	.cfi_def_cfa_offset 212
	push	DWORD PTR [eax]
	.cfi_def_cfa_offset 216
	push	OFFSET FLAT:.LC3
	.cfi_def_cfa_offset 220
	push	1
	.cfi_def_cfa_offset 224
	call	__printf_chk
	add	esp, 208
	.cfi_def_cfa_offset 16
.L277:
	add	esp, 12
	.cfi_def_cfa_offset 4
	ret
	.cfi_endproc
.LFE51:
	.size	Tagha_PrintVMState, .-Tagha_PrintVMState
	.p2align 4,,15
	.globl	Tagha_RegisterNatives
	.type	Tagha_RegisterNatives, @function
Tagha_RegisterNatives:
.LFB52:
	.cfi_startproc
	push	ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	push	edi
	.cfi_def_cfa_offset 12
	.cfi_offset 7, -12
	push	esi
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	push	ebx
	.cfi_def_cfa_offset 20
	.cfi_offset 3, -20
	sub	esp, 44
	.cfi_def_cfa_offset 64
	mov	edx, DWORD PTR [esp+64]
	mov	eax, DWORD PTR [esp+68]
	test	edx, edx
	je	.L292
	test	eax, eax
	je	.L292
	mov	ecx, DWORD PTR [eax+4]
	test	ecx, ecx
	mov	DWORD PTR [esp+20], ecx
	je	.L286
	mov	ebp, DWORD PTR [eax]
	test	ebp, ebp
	je	.L286
	mov	ebx, DWORD PTR [edx+200]
	mov	DWORD PTR [esp+16], eax
	lea	esi, [ebx+15]
	mov	DWORD PTR [esp+24], ebx
	mov	DWORD PTR [esp+28], esi
	.p2align 4,,10
	.p2align 3
.L291:
	mov	edi, DWORD PTR [esp+24]
	test	edi, edi
	je	.L287
	mov	edx, DWORD PTR [edi+11]
	test	edx, edx
	mov	DWORD PTR [esp+12], edx
	je	.L287
	mov	ecx, DWORD PTR [esp+28]
	lea	eax, [edx-1]
	sub	esp, 8
	.cfi_def_cfa_offset 72
	and	eax, 3
	lea	edi, [ecx+9]
	mov	DWORD PTR [esp+16], eax
	mov	ebx, DWORD PTR [ecx+1]
	mov	esi, DWORD PTR [ecx+5]
	push	edi
	.cfi_def_cfa_offset 76
	push	ebp
	.cfi_def_cfa_offset 80
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 64
	test	eax, eax
	mov	edx, DWORD PTR [esp+8]
	je	.L330
	add	esi, ebx
	mov	DWORD PTR [esp+8], 1
	add	esi, edi
	cmp	DWORD PTR [esp+12], 1
	je	.L287
	test	edx, edx
	je	.L290
	cmp	edx, 1
	je	.L321
	cmp	edx, 2
	je	.L322
	mov	ebx, DWORD PTR [esi+1]
	lea	edi, [esi+9]
	mov	esi, DWORD PTR [esi+5]
	sub	esp, 8
	.cfi_def_cfa_offset 72
	push	edi
	.cfi_def_cfa_offset 76
	push	ebp
	.cfi_def_cfa_offset 80
	add	esi, ebx
	call	strcmp
	add	esi, edi
	add	esp, 16
	.cfi_def_cfa_offset 64
	test	eax, eax
	mov	DWORD PTR [esp+8], 2
	je	.L330
.L322:
	lea	edi, [esi+9]
	sub	esp, 8
	.cfi_def_cfa_offset 72
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 76
	push	ebp
	.cfi_def_cfa_offset 80
	add	esi, ebx
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 64
	add	esi, edi
	add	DWORD PTR [esp+8], 1
	test	eax, eax
	je	.L330
.L321:
	lea	edi, [esi+9]
	sub	esp, 8
	.cfi_def_cfa_offset 72
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 76
	push	ebp
	.cfi_def_cfa_offset 80
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 64
	test	eax, eax
	je	.L330
	add	DWORD PTR [esp+8], 1
	add	esi, ebx
	mov	eax, DWORD PTR [esp+8]
	add	esi, edi
	cmp	DWORD PTR [esp+12], eax
	je	.L287
.L290:
	lea	edi, [esi+9]
	sub	esp, 8
	.cfi_def_cfa_offset 72
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 76
	push	ebp
	.cfi_def_cfa_offset 80
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 64
	test	eax, eax
	je	.L330
	add	esi, ebx
	sub	esp, 8
	.cfi_def_cfa_offset 72
	add	esi, edi
	lea	edi, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 76
	push	ebp
	.cfi_def_cfa_offset 80
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 64
	test	eax, eax
	je	.L330
	add	esi, ebx
	sub	esp, 8
	.cfi_def_cfa_offset 72
	add	esi, edi
	lea	edi, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 76
	push	ebp
	.cfi_def_cfa_offset 80
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 64
	test	eax, eax
	je	.L330
	add	esi, ebx
	sub	esp, 8
	.cfi_def_cfa_offset 72
	add	esi, edi
	lea	edi, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	.cfi_def_cfa_offset 76
	push	ebp
	.cfi_def_cfa_offset 80
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 64
	test	eax, eax
	je	.L330
	add	DWORD PTR [esp+8], 4
	add	esi, ebx
	mov	eax, DWORD PTR [esp+8]
	add	esi, edi
	cmp	DWORD PTR [esp+12], eax
	jne	.L290
	.p2align 4,,10
	.p2align 3
.L287:
	add	DWORD PTR [esp+16], 8
	mov	ecx, DWORD PTR [esp+16]
	mov	edx, DWORD PTR [ecx+4]
	test	edx, edx
	mov	DWORD PTR [esp+20], edx
	je	.L286
	mov	ebp, DWORD PTR [ecx]
	test	ebp, ebp
	jne	.L291
.L286:
	add	esp, 44
	.cfi_remember_state
	.cfi_def_cfa_offset 20
	mov	eax, 1
	pop	ebx
	.cfi_restore 3
	.cfi_def_cfa_offset 16
	pop	esi
	.cfi_restore 6
	.cfi_def_cfa_offset 12
	pop	edi
	.cfi_restore 7
	.cfi_def_cfa_offset 8
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa_offset 4
	ret
	.p2align 4,,10
	.p2align 3
.L330:
	.cfi_restore_state
	add	ebx, edi
	je	.L287
	mov	ebp, DWORD PTR [esp+20]
	mov	DWORD PTR [ebx], ebp
	jmp	.L287
	.p2align 4,,10
	.p2align 3
.L292:
	add	esp, 44
	.cfi_def_cfa_offset 20
	xor	eax, eax
	pop	ebx
	.cfi_restore 3
	.cfi_def_cfa_offset 16
	pop	esi
	.cfi_restore 6
	.cfi_def_cfa_offset 12
	pop	edi
	.cfi_restore 7
	.cfi_def_cfa_offset 8
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa_offset 4
	ret
	.cfi_endproc
.LFE52:
	.size	Tagha_RegisterNatives, .-Tagha_RegisterNatives
	.globl	__udivdi3
	.globl	__umoddi3
	.p2align 4,,15
	.globl	Tagha_Exec
	.type	Tagha_Exec, @function
Tagha_Exec:
.LFB58:
	.cfi_startproc
	push	ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	mov	ebp, esp
	.cfi_def_cfa_register 5
	push	edi
	push	esi
	push	ebx
	sub	esp, 60
	.cfi_offset 7, -12
	.cfi_offset 6, -16
	.cfi_offset 3, -20
	mov	edi, DWORD PTR [ebp+8]
	mov	eax, DWORD PTR gs:20
	mov	DWORD PTR [ebp-28], eax
	xor	eax, eax
	test	edi, edi
	je	.L870
	mov	eax, DWORD PTR [edi+200]
	test	eax, eax
	je	.L342
	mov	edx, DWORD PTR [edi+184]
	mov	ecx, DWORD PTR [edi+176]
	mov	ebx, DWORD PTR [edi+180]
	mov	esi, DWORD PTR [edi+192]
	mov	DWORD PTR [ebp-56], edx
	mov	DWORD PTR [edi+184], ecx
	mov	DWORD PTR [edi+188], ebx
	.p2align 4,,10
	.p2align 3
.L845:
	movzx	eax, WORD PTR [esi]
	lea	ecx, [esi+2]
	movzx	edx, al
	cmp	dl, 46
	ja	.L342
.L1360:
	mov	esi, ecx
	jmp	[DWORD PTR dispatch.3852[0+edx*4]]
	.p2align 4,,10
	.p2align 3
.L345:
	movzx	eax, ah
	test	al, 1
	jne	.L1357
	test	al, 2
	je	.L348
	mov	ebx, DWORD PTR [edi+176]
	movzx	eax, BYTE PTR [esi]
	add	esi, 1
	lea	ecx, [ebx-8]
	mov	DWORD PTR [edi+176], ecx
	mov	edx, DWORD PTR [edi+4+eax*8]
	mov	ecx, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [ebx-4], edx
	mov	DWORD PTR [ebx-8], ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L350:
	movzx	eax, ah
	test	al, 2
	jne	.L1358
	test	al, 4
	je	.L845
	movzx	ebx, BYTE PTR [esi]
	mov	eax, DWORD PTR [edi+176]
	add	esi, 5
	mov	ecx, DWORD PTR [esi-4]
	lea	edx, [eax+8]
	add	ecx, DWORD PTR [edi+ebx*8]
	mov	ebx, DWORD PTR [eax+4]
	mov	eax, DWORD PTR [eax]
	mov	DWORD PTR [edi+176], edx
	mov	DWORD PTR [ecx], eax
	mov	DWORD PTR [ecx+4], ebx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L353:
	movzx	eax, ah
	test	al, 1
	je	.L354
	movzx	ecx, BYTE PTR [esi]
	lea	eax, [esi+9]
	mov	esi, DWORD PTR [esi+1]
	mov	DWORD PTR [ebp-64], eax
	mov	DWORD PTR [ebp-48], esi
	lea	ebx, [edi+ecx*8]
	mov	ecx, DWORD PTR [edi+200]
	mov	DWORD PTR [ebp-60], ebx
	test	ecx, ecx
	je	.L873
	mov	edx, DWORD PTR [ecx+7]
	lea	edx, [ecx+11+edx]
	mov	eax, DWORD PTR [edx]
	cmp	esi, eax
	mov	DWORD PTR [ebp-52], eax
	jnb	.L873
	test	eax, eax
	mov	ebx, eax
	je	.L873
	test	esi, esi
	lea	ecx, [edx+13]
	mov	eax, DWORD PTR [edx+5]
	mov	edx, DWORD PTR [edx+9]
	je	.L356
	sub	ebx, 1
	xor	esi, esi
	and	ebx, 3
	je	.L1267
	add	edx, eax
	mov	esi, 1
	add	edx, ecx
	cmp	DWORD PTR [ebp-48], 1
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L356
	cmp	ebx, 1
	je	.L1267
	cmp	ebx, 2
	je	.L1161
	add	edx, eax
	mov	esi, 2
	add	edx, ecx
	cmp	DWORD PTR [ebp-48], 2
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L356
.L1161:
	add	edx, eax
	add	esi, 1
	add	edx, ecx
	cmp	DWORD PTR [ebp-48], esi
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L356
.L1267:
	mov	ebx, DWORD PTR [ebp-48]
	mov	DWORD PTR [ebp-48], edi
.L357:
	add	eax, edx
	lea	edi, [esi+1]
	add	eax, ecx
	cmp	DWORD PTR [ebp-52], edi
	je	.L1359
	cmp	ebx, edi
	lea	ecx, [eax+9]
	mov	edx, DWORD PTR [eax+5]
	mov	eax, DWORD PTR [eax+1]
	je	.L1269
	add	eax, edx
	lea	edx, [esi+2]
	add	eax, ecx
	cmp	ebx, edx
	lea	ecx, [eax+9]
	mov	edi, DWORD PTR [eax+5]
	mov	eax, DWORD PTR [eax+1]
	je	.L1269
	add	eax, edi
	lea	edi, [esi+3]
	add	eax, ecx
	cmp	ebx, edi
	lea	ecx, [eax+9]
	mov	edx, DWORD PTR [eax+5]
	mov	eax, DWORD PTR [eax+1]
	je	.L1269
	add	edx, eax
	add	esi, 4
	add	edx, ecx
	cmp	ebx, esi
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	jne	.L357
.L1269:
	mov	edi, DWORD PTR [ebp-48]
.L356:
	add	eax, ecx
.L355:
	mov	esi, DWORD PTR [ebp-60]
	mov	DWORD PTR [esi], eax
	mov	esi, DWORD PTR [ebp-64]
	movzx	eax, WORD PTR [esi]
	lea	ecx, [esi+2]
	movzx	edx, al
	cmp	dl, 46
	jbe	.L1360
	.p2align 4,,10
	.p2align 3
.L342:
	mov	DWORD PTR [edi+208], -1
	mov	eax, -1
	jmp	.L337
	.p2align 4,,10
	.p2align 3
.L361:
	movzx	eax, ah
	test	al, 8
	je	.L362
	test	al, 1
	je	.L363
	movzx	ecx, BYTE PTR [esi]
	mov	eax, DWORD PTR [esi+1]
	add	esi, 9
	mov	edx, DWORD PTR [esi-4]
	mov	DWORD PTR [edi+ecx*8], eax
	mov	DWORD PTR [edi+4+ecx*8], edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L377:
	movzx	eax, ah
	test	al, 8
	je	.L378
	test	al, 1
	je	.L379
	movzx	ecx, BYTE PTR [esi]
	mov	eax, DWORD PTR [esi+1]
	mov	ebx, DWORD PTR [esi+5]
	add	DWORD PTR [edi+ecx*8], eax
	adc	DWORD PTR [edi+4+ecx*8], ebx
	add	esi, 9
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L393:
	movzx	eax, ah
	test	al, 8
	je	.L394
	test	al, 1
	je	.L395
	movzx	ecx, BYTE PTR [esi]
	mov	eax, DWORD PTR [esi+1]
	mov	ebx, DWORD PTR [esi+5]
	sub	DWORD PTR [edi+ecx*8], eax
	sbb	DWORD PTR [edi+4+ecx*8], ebx
	add	esi, 9
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L409:
	movzx	eax, ah
	test	al, 8
	je	.L410
	test	al, 1
	je	.L411
	movzx	eax, BYTE PTR [esi]
	add	esi, 9
	lea	ebx, [edi+eax*8]
	mov	ecx, DWORD PTR [ebx+4]
	mov	eax, DWORD PTR [ebx]
	imul	ecx, DWORD PTR [esi-8]
	mov	edx, ecx
	mov	ecx, DWORD PTR [esi-4]
	imul	eax, ecx
	lea	ecx, [edx+eax]
	mov	eax, DWORD PTR [esi-8]
	mul	DWORD PTR [ebx]
	add	edx, ecx
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L425:
	movzx	eax, ah
	test	al, 8
	je	.L426
	test	al, 1
	je	.L427
	movzx	ebx, BYTE PTR [esi]
	lea	eax, [esi+9]
	mov	DWORD PTR [ebp-48], eax
	push	DWORD PTR [esi+5]
	push	DWORD PTR [esi+1]
	lea	ebx, [edi+ebx*8]
	push	DWORD PTR [ebx+4]
	push	DWORD PTR [ebx]
	call	__udivdi3
	add	esp, 16
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	mov	esi, DWORD PTR [ebp-48]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L441:
	movzx	eax, ah
	test	al, 8
	je	.L442
	test	al, 1
	je	.L443
	movzx	ebx, BYTE PTR [esi]
	lea	eax, [esi+9]
	mov	DWORD PTR [ebp-48], eax
	push	DWORD PTR [esi+5]
	push	DWORD PTR [esi+1]
	lea	ebx, [edi+ebx*8]
	push	DWORD PTR [ebx+4]
	push	DWORD PTR [ebx]
	call	__umoddi3
	add	esp, 16
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	mov	esi, DWORD PTR [ebp-48]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L457:
	movzx	eax, ah
	test	al, 8
	je	.L458
	test	al, 1
	je	.L459
	movzx	edx, BYTE PTR [esi]
	mov	ecx, DWORD PTR [esi+1]
	add	esi, 9
	mov	ebx, DWORD PTR [esi-4]
	lea	eax, [edi+edx*8]
	and	DWORD PTR [eax], ecx
	and	DWORD PTR [eax+4], ebx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L473:
	movzx	eax, ah
	test	al, 8
	je	.L474
	test	al, 1
	je	.L475
	movzx	edx, BYTE PTR [esi]
	mov	ecx, DWORD PTR [esi+1]
	add	esi, 9
	mov	ebx, DWORD PTR [esi-4]
	lea	eax, [edi+edx*8]
	or	DWORD PTR [eax], ecx
	or	DWORD PTR [eax+4], ebx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L489:
	movzx	eax, ah
	test	al, 8
	je	.L490
	test	al, 1
	je	.L491
	movzx	edx, BYTE PTR [esi]
	mov	ecx, DWORD PTR [esi+1]
	add	esi, 9
	mov	ebx, DWORD PTR [esi-4]
	lea	eax, [edi+edx*8]
	xor	DWORD PTR [eax], ecx
	xor	DWORD PTR [eax+4], ebx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L505:
	movzx	eax, ah
	lea	edx, [esi+1]
	movzx	ecx, BYTE PTR [esi]
	test	al, 2
	jne	.L1361
	test	al, 4
	je	.L866
	lea	edx, [esi+5]
	mov	esi, DWORD PTR [esi+1]
	add	esi, DWORD PTR [edi+ecx*8]
	test	al, 16
	jne	.L1362
	test	al, 32
	je	.L509
	not	WORD PTR [esi]
	.p2align 4,,10
	.p2align 3
.L866:
	movzx	eax, WORD PTR [edx]
	lea	esi, [edx+2]
	movzx	edx, al
	cmp	dl, 46
	jbe	.L1271
	jmp	.L342
	.p2align 4,,10
	.p2align 3
.L511:
	movzx	eax, ah
	test	al, 8
	je	.L512
	test	al, 1
	je	.L513
	movzx	eax, BYTE PTR [esi]
	mov	ecx, DWORD PTR [esi+1]
	lea	ebx, [edi+eax*8]
	mov	eax, DWORD PTR [ebx]
	mov	edx, DWORD PTR [ebx+4]
	shld	edx, eax, cl
	sal	eax, cl
	test	cl, 32
	je	.L1427
	mov	edx, eax
	xor	eax, eax
.L1427:
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	add	esi, 9
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L527:
	movzx	eax, ah
	test	al, 8
	je	.L528
	test	al, 1
	je	.L529
	movzx	eax, BYTE PTR [esi]
	mov	ecx, DWORD PTR [esi+1]
	lea	ebx, [edi+eax*8]
	mov	edx, DWORD PTR [ebx+4]
	mov	eax, DWORD PTR [ebx]
	shrd	eax, edx, cl
	shr	edx, cl
	test	cl, 32
	je	.L1426
	mov	eax, edx
	xor	edx, edx
.L1426:
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	add	esi, 9
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L543:
	movzx	eax, ah
	lea	edx, [esi+1]
	movzx	ecx, BYTE PTR [esi]
	test	al, 2
	jne	.L1363
	test	al, 4
	je	.L866
	lea	edx, [esi+5]
	mov	esi, DWORD PTR [esi+1]
	add	esi, DWORD PTR [edi+ecx*8]
	test	al, 16
	jne	.L1364
	test	al, 32
	je	.L547
	add	WORD PTR [esi], 1
	jmp	.L866
	.p2align 4,,10
	.p2align 3
.L549:
	movzx	eax, ah
	lea	edx, [esi+1]
	movzx	ecx, BYTE PTR [esi]
	test	al, 2
	jne	.L1365
	test	al, 4
	je	.L866
	lea	edx, [esi+5]
	mov	esi, DWORD PTR [esi+1]
	add	esi, DWORD PTR [edi+ecx*8]
	test	al, 16
	jne	.L1366
	test	al, 32
	je	.L553
	sub	WORD PTR [esi], 1
	jmp	.L866
	.p2align 4,,10
	.p2align 3
.L555:
	movzx	eax, ah
	lea	edx, [esi+1]
	movzx	ecx, BYTE PTR [esi]
	test	al, 2
	jne	.L1367
	test	al, 4
	je	.L866
	lea	edx, [esi+5]
	mov	esi, DWORD PTR [esi+1]
	add	esi, DWORD PTR [edi+ecx*8]
	test	al, 16
	jne	.L1368
	test	al, 32
	je	.L559
	neg	WORD PTR [esi]
	jmp	.L866
	.p2align 4,,10
	.p2align 3
.L561:
	movzx	eax, ah
	test	al, 8
	je	.L562
	test	al, 1
	je	.L563
	movzx	eax, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	edx, 1
	mov	ebx, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [ebp-48], ebx
	mov	ebx, DWORD PTR [esi+5]
	cmp	DWORD PTR [edi+4+eax*8], ebx
	jl	.L564
	jg	.L565
	mov	eax, DWORD PTR [ebp-48]
	cmp	eax, DWORD PTR [esi+1]
	jnb	.L565
.L564:
	movzx	ebx, BYTE PTR [edi+212]
	mov	esi, ecx
	and	ebx, -2
	or	edx, ebx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L587:
	movzx	eax, ah
	test	al, 8
	je	.L588
	test	al, 1
	je	.L589
	movzx	eax, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	edx, 1
	mov	ebx, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [ebp-48], ebx
	mov	ebx, DWORD PTR [esi+5]
	cmp	DWORD PTR [edi+4+eax*8], ebx
	jg	.L590
	jl	.L591
	mov	eax, DWORD PTR [ebp-48]
	cmp	eax, DWORD PTR [esi+1]
	jbe	.L591
.L590:
	movzx	ebx, BYTE PTR [edi+212]
	mov	esi, ecx
	and	ebx, -2
	or	edx, ebx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L613:
	movzx	eax, ah
	test	al, 8
	je	.L614
	test	al, 1
	je	.L615
	movzx	eax, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	edx, 1
	mov	ebx, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [ebp-48], ebx
	mov	ebx, DWORD PTR [esi+5]
	cmp	DWORD PTR [edi+4+eax*8], ebx
	jb	.L616
	ja	.L617
	mov	eax, DWORD PTR [ebp-48]
	cmp	eax, DWORD PTR [esi+1]
	jnb	.L617
.L616:
	movzx	ebx, BYTE PTR [edi+212]
	mov	esi, ecx
	and	ebx, -2
	or	edx, ebx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L639:
	movzx	eax, ah
	test	al, 8
	je	.L640
	test	al, 1
	je	.L641
	movzx	eax, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	edx, 1
	mov	ebx, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [ebp-48], ebx
	mov	ebx, DWORD PTR [esi+5]
	cmp	DWORD PTR [edi+4+eax*8], ebx
	ja	.L642
	jb	.L643
	mov	eax, DWORD PTR [ebp-48]
	cmp	eax, DWORD PTR [esi+1]
	jbe	.L643
.L642:
	movzx	ebx, BYTE PTR [edi+212]
	mov	esi, ecx
	and	ebx, -2
	or	edx, ebx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L665:
	movzx	eax, ah
	test	al, 8
	je	.L666
	test	al, 1
	je	.L667
	movzx	ebx, BYTE PTR [esi]
	movzx	eax, BYTE PTR [edi+212]
	mov	ecx, DWORD PTR [edi+ebx*8]
	mov	edx, DWORD PTR [edi+4+ebx*8]
	xor	ecx, DWORD PTR [esi+1]
	xor	edx, DWORD PTR [esi+5]
	or	ecx, edx
	sete	bl
	and	eax, -2
	add	esi, 9
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L681:
	movzx	eax, ah
	test	al, 8
	je	.L682
	test	al, 1
	je	.L683
	movzx	ebx, BYTE PTR [esi]
	movzx	eax, BYTE PTR [edi+212]
	mov	ecx, DWORD PTR [edi+ebx*8]
	mov	edx, DWORD PTR [edi+4+ebx*8]
	xor	ecx, DWORD PTR [esi+1]
	xor	edx, DWORD PTR [esi+5]
	or	ecx, edx
	setne	bl
	and	eax, -2
	add	esi, 9
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L697:
	movzx	eax, ah
	test	al, 1
	jne	.L1369
	test	al, 2
	je	.L700
	movzx	eax, BYTE PTR [esi]
	mov	ecx, DWORD PTR [edi+eax*8]
	lea	esi, [esi+1+ecx]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L704:
	movzx	eax, ah
	test	al, 1
	jne	.L1370
	test	al, 2
	je	.L707
	movzx	ebx, BYTE PTR [esi]
	add	esi, 1
	test	BYTE PTR [edi+212], 1
	jne	.L845
	add	esi, DWORD PTR [edi+ebx*8]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L711:
	movzx	eax, ah
	test	al, 1
	jne	.L1371
	test	al, 2
	je	.L714
	movzx	ebx, BYTE PTR [esi]
	add	esi, 1
	test	BYTE PTR [edi+212], 1
	je	.L845
	add	esi, DWORD PTR [edi+ebx*8]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L718:
	movzx	eax, ah
	test	al, 1
	je	.L719
	mov	eax, DWORD PTR [esi]
	lea	edx, [esi+8]
	mov	DWORD PTR [ebp-60], edx
	add	eax, -1
.L720:
	mov	edx, DWORD PTR [edi+200]
	mov	DWORD PTR [ebp-48], eax
	test	edx, edx
	je	.L722
	mov	ebx, DWORD PTR [edx+11]
	mov	esi, DWORD PTR [ebp-48]
	cmp	ebx, esi
	mov	DWORD PTR [ebp-52], ebx
	jbe	.L722
	test	esi, esi
	lea	ecx, [edx+24]
	mov	eax, DWORD PTR [edx+16]
	mov	edx, DWORD PTR [edx+20]
	je	.L723
	sub	ebx, 1
	xor	esi, esi
	and	ebx, 3
	je	.L1263
	add	edx, eax
	mov	esi, 1
	add	edx, ecx
	cmp	esi, DWORD PTR [ebp-48]
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L723
	cmp	ebx, 1
	je	.L1263
	cmp	ebx, 2
	je	.L1157
	add	edx, eax
	mov	esi, 2
	add	edx, ecx
	cmp	esi, DWORD PTR [ebp-48]
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L723
.L1157:
	add	edx, eax
	add	esi, 1
	add	edx, ecx
	cmp	esi, DWORD PTR [ebp-48]
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L723
.L1263:
	mov	ebx, DWORD PTR [ebp-48]
	mov	DWORD PTR [ebp-48], edi
	jmp	.L724
	.p2align 4,,10
	.p2align 3
.L726:
	cmp	edi, ebx
	lea	ecx, [eax+9]
	mov	edx, DWORD PTR [eax+5]
	mov	eax, DWORD PTR [eax+1]
	je	.L1265
	add	eax, edx
	lea	edx, [esi+2]
	add	eax, ecx
	cmp	edx, ebx
	lea	ecx, [eax+9]
	mov	edi, DWORD PTR [eax+5]
	mov	eax, DWORD PTR [eax+1]
	je	.L1265
	add	eax, edi
	lea	edi, [esi+3]
	add	eax, ecx
	cmp	edi, ebx
	lea	ecx, [eax+9]
	mov	edx, DWORD PTR [eax+5]
	mov	eax, DWORD PTR [eax+1]
	je	.L1265
	add	edx, eax
	add	esi, 4
	add	edx, ecx
	cmp	esi, ebx
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L1265
.L724:
	add	eax, edx
	lea	edi, [esi+1]
	add	eax, ecx
	cmp	DWORD PTR [ebp-52], edi
	jne	.L726
	mov	edi, DWORD PTR [ebp-48]
	jmp	.L722
	.p2align 4,,10
	.p2align 3
.L727:
	mov	edx, DWORD PTR [edi+184]
	mov	ecx, DWORD PTR [edi+188]
	lea	esi, [edx+8]
	mov	DWORD PTR [edi+180], ecx
	mov	ecx, DWORD PTR [edx]
	cmp	DWORD PTR [ebp-56], ecx
	mov	eax, edx
	mov	DWORD PTR [edi+176], esi
	mov	esi, DWORD PTR [edx+4]
	mov	DWORD PTR [edi+184], ecx
	mov	DWORD PTR [edi+188], esi
	je	.L338
	add	edx, 16
	mov	DWORD PTR [edi+176], edx
	mov	edx, DWORD PTR [eax+8]
	movzx	eax, WORD PTR [edx]
	lea	esi, [edx+2]
	movzx	edx, al
	cmp	dl, 46
	ja	.L342
	.p2align 4,,10
	.p2align 3
.L1271:
	mov	ebx, DWORD PTR dispatch.3852[0+edx*4]
	jmp	ebx
	.p2align 4,,10
	.p2align 3
.L728:
	mov	ecx, DWORD PTR [esi]
	movzx	eax, ah
	test	al, 1
	mov	DWORD PTR [ebp-64], ecx
	je	.L729
	mov	ecx, DWORD PTR [esi+4]
	add	esi, 12
	not	ecx
	mov	DWORD PTR [ebp-48], ecx
.L730:
	mov	edx, DWORD PTR [edi+200]
	test	edx, edx
	je	.L338
	mov	ebx, DWORD PTR [edx+11]
	cmp	DWORD PTR [ebp-48], ebx
	mov	DWORD PTR [ebp-60], ebx
	jnb	.L338
	test	ebx, ebx
	je	.L338
	cmp	DWORD PTR [ebp-48], 0
	lea	ecx, [edx+24]
	mov	eax, DWORD PTR [edx+16]
	mov	edx, DWORD PTR [edx+20]
	je	.L733
	sub	ebx, 1
	and	ebx, 3
	je	.L1372
	add	edx, eax
	mov	DWORD PTR [ebp-52], 1
	add	edx, ecx
	cmp	DWORD PTR [ebp-48], 1
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L733
	cmp	ebx, 1
	je	.L1258
	cmp	ebx, 2
	je	.L1153
	add	edx, eax
	mov	DWORD PTR [ebp-52], 2
	add	edx, ecx
	cmp	DWORD PTR [ebp-48], 2
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L733
.L1153:
	add	edx, eax
	add	DWORD PTR [ebp-52], 1
	add	edx, ecx
	mov	ebx, DWORD PTR [ebp-52]
	cmp	DWORD PTR [ebp-48], ebx
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L733
	mov	DWORD PTR [ebp-68], esi
	mov	ebx, DWORD PTR [ebp-52]
	mov	esi, DWORD PTR [ebp-48]
	mov	DWORD PTR [ebp-52], edi
	jmp	.L734
	.p2align 4,,10
	.p2align 3
.L736:
	cmp	esi, edi
	lea	ecx, [eax+9]
	mov	edx, DWORD PTR [eax+5]
	mov	eax, DWORD PTR [eax+1]
	je	.L1261
	add	eax, edx
	lea	edx, [ebx+2]
	add	eax, ecx
	cmp	esi, edx
	lea	ecx, [eax+9]
	mov	edi, DWORD PTR [eax+5]
	mov	eax, DWORD PTR [eax+1]
	je	.L1261
	add	eax, edi
	lea	edi, [ebx+3]
	add	eax, ecx
	cmp	esi, edi
	lea	ecx, [eax+9]
	mov	edx, DWORD PTR [eax+5]
	mov	eax, DWORD PTR [eax+1]
	je	.L1261
	add	edx, eax
	add	ebx, 4
	add	edx, ecx
	cmp	esi, ebx
	lea	ecx, [edx+9]
	mov	eax, DWORD PTR [edx+1]
	mov	edx, DWORD PTR [edx+5]
	je	.L1261
.L734:
	add	eax, edx
	lea	edi, [ebx+1]
	add	eax, ecx
	cmp	DWORD PTR [ebp-60], edi
	jne	.L736
	mov	edi, DWORD PTR [ebp-52]
	.p2align 4,,10
	.p2align 3
.L338:
	mov	eax, DWORD PTR [edi]
.L337:
	mov	ebx, DWORD PTR [ebp-28]
	xor	ebx, DWORD PTR gs:20
	jne	.L1373
	lea	esp, [ebp-12]
	pop	ebx
	.cfi_remember_state
	.cfi_restore 3
	pop	esi
	.cfi_restore 6
	pop	edi
	.cfi_restore 7
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.p2align 4,,10
	.p2align 3
.L739:
	.cfi_restore_state
	test	ah, 2
	je	.L845
	movzx	edx, BYTE PTR [esi]
	add	esi, 1
	fld	DWORD PTR [edi+edx*8]
	fstp	QWORD PTR [edi+edx*8]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L741:
	test	ah, 2
	je	.L845
	movzx	ecx, BYTE PTR [esi]
	add	esi, 1
	lea	ebx, [edi+ecx*8]
	fld	QWORD PTR [ebx]
	mov	DWORD PTR [ebx+4], 0
	fstp	DWORD PTR [ebx]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L743:
	test	ah, 2
	je	.L845
	lea	edx, [esi+1]
	movzx	esi, BYTE PTR [esi]
	lea	eax, [edi+esi*8]
	mov	ecx, DWORD PTR [eax+4]
	fild	QWORD PTR [eax]
	test	ecx, ecx
	js	.L1374
.L745:
	mov	esi, edx
	fstp	QWORD PTR [eax]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L746:
	test	ah, 2
	je	.L845
	movzx	ebx, BYTE PTR [esi]
	lea	ecx, [esi+1]
	lea	esi, [edi+ebx*8]
	mov	edx, DWORD PTR [esi+4]
	mov	eax, DWORD PTR [esi]
	mov	DWORD PTR [esi+4], 0
	mov	DWORD PTR [esi], 0
	mov	DWORD PTR [ebp-48], eax
	mov	DWORD PTR [ebp-44], edx
	test	edx, edx
	fild	QWORD PTR [ebp-48]
	js	.L1375
.L748:
	fstp	DWORD PTR [esi]
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L749:
	movzx	eax, ah
	test	al, 8
	je	.L750
	test	al, 1
	je	.L751
	mov	ebx, DWORD PTR [esi+1]
	movzx	edx, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	esi, DWORD PTR [esi+5]
	test	al, 64
	mov	DWORD PTR [ebp-48], ebx
	mov	DWORD PTR [ebp-44], esi
	je	.L752
	lea	edx, [edi+edx*8]
	fld	DWORD PTR [ebp-48]
	fadd	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
.L1293:
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L762:
	movzx	eax, ah
	test	al, 8
	je	.L763
	test	al, 1
	je	.L764
	mov	ebx, DWORD PTR [esi+1]
	movzx	edx, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	esi, DWORD PTR [esi+5]
	test	al, 64
	mov	DWORD PTR [ebp-48], ebx
	mov	DWORD PTR [ebp-44], esi
	je	.L765
	lea	edx, [edi+edx*8]
	fld	DWORD PTR [ebp-48]
	fsubr	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
.L1294:
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L775:
	movzx	eax, ah
	test	al, 8
	je	.L776
	test	al, 1
	je	.L777
	mov	ebx, DWORD PTR [esi+1]
	movzx	edx, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	esi, DWORD PTR [esi+5]
	test	al, 64
	mov	DWORD PTR [ebp-48], ebx
	mov	DWORD PTR [ebp-44], esi
	je	.L778
	lea	edx, [edi+edx*8]
	fld	DWORD PTR [ebp-48]
	fmul	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
.L1295:
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L788:
	movzx	eax, ah
	test	al, 8
	je	.L789
	test	al, 1
	je	.L790
	mov	ebx, DWORD PTR [esi+1]
	movzx	edx, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	esi, DWORD PTR [esi+5]
	test	al, 64
	mov	DWORD PTR [ebp-48], ebx
	mov	DWORD PTR [ebp-44], esi
	jne	.L1376
	test	al, al
	jns	.L1296
	lea	eax, [edi+edx*8]
	fld	QWORD PTR [ebp-48]
	fdivr	QWORD PTR [eax]
	fstp	QWORD PTR [eax]
.L1296:
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L801:
	movzx	eax, ah
	test	al, 8
	je	.L802
	test	al, 1
	je	.L803
	test	al, 64
	movzx	edx, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	je	.L804
	mov	DWORD PTR [ebp-48], ebx
	fld	DWORD PTR [ebp-48]
	fld	DWORD PTR [edi+edx*8]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
.L1298:
	movzx	eax, BYTE PTR [edi+212]
	seta	dl
	and	eax, -2
	or	eax, edx
	mov	BYTE PTR [edi+212], al
.L812:
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L814:
	movzx	eax, ah
	test	al, 8
	je	.L815
	test	al, 1
	je	.L816
	test	al, 64
	movzx	edx, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	jne	.L1377
	test	al, al
	jns	.L825
	fld	QWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], ebx
	mov	DWORD PTR [ebp-44], esi
	fld	QWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1301
	.p2align 4,,10
	.p2align 3
.L827:
	movzx	eax, ah
	test	al, 8
	je	.L828
	test	al, 1
	je	.L829
	test	al, 64
	movzx	edx, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	jne	.L1378
	test	al, al
	jns	.L838
	fld	QWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], ebx
	mov	DWORD PTR [ebp-44], esi
	fld	QWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1305
	.p2align 4,,10
	.p2align 3
.L840:
	movzx	eax, ah
	test	al, 8
	je	.L841
	test	al, 1
	je	.L842
	test	al, 64
	movzx	edx, BYTE PTR [esi]
	lea	ecx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	jne	.L1379
	test	al, al
	jns	.L851
	fld	QWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], ebx
	mov	DWORD PTR [ebp-44], esi
	fld	QWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1309
	.p2align 4,,10
	.p2align 3
.L853:
	movzx	eax, ah
	lea	edx, [esi+1]
	movzx	ecx, BYTE PTR [esi]
	test	al, 2
	je	.L854
	test	al, 64
	je	.L855
	lea	ecx, [edi+ecx*8]
	fld1
	fadd	DWORD PTR [ecx]
	fstp	DWORD PTR [ecx]
	jmp	.L866
	.p2align 4,,10
	.p2align 3
.L858:
	movzx	eax, ah
	lea	edx, [esi+1]
	movzx	ecx, BYTE PTR [esi]
	test	al, 2
	je	.L859
	test	al, 64
	je	.L860
	lea	ecx, [edi+ecx*8]
	fld1
	fsubr	DWORD PTR [ecx]
	fstp	DWORD PTR [ecx]
	jmp	.L866
	.p2align 4,,10
	.p2align 3
.L863:
	movzx	eax, ah
	lea	edx, [esi+1]
	movzx	ecx, BYTE PTR [esi]
	test	al, 2
	je	.L864
	test	al, 64
	je	.L865
	lea	eax, [edi+ecx*8]
	fld	DWORD PTR [eax]
	fchs
	fstp	DWORD PTR [eax]
	jmp	.L866
	.p2align 4,,10
	.p2align 3
.L864:
	test	al, 4
	je	.L866
	lea	edx, [esi+5]
	mov	esi, DWORD PTR [esi+1]
	add	esi, DWORD PTR [edi+ecx*8]
	test	al, 64
	jne	.L1380
	test	al, al
	jns	.L866
	fld	QWORD PTR [esi]
	fchs
	fstp	QWORD PTR [esi]
	jmp	.L866
	.p2align 4,,10
	.p2align 3
.L859:
	test	al, 4
	je	.L866
	mov	ebx, DWORD PTR [esi+1]
	add	ebx, DWORD PTR [edi+ecx*8]
	test	al, 64
	lea	edx, [esi+5]
	jne	.L1381
	test	al, al
	jns	.L866
	fld1
	fsubr	QWORD PTR [ebx]
	fstp	QWORD PTR [ebx]
	jmp	.L866
	.p2align 4,,10
	.p2align 3
.L854:
	test	al, 4
	je	.L866
	mov	ebx, DWORD PTR [esi+1]
	add	ebx, DWORD PTR [edi+ecx*8]
	test	al, 64
	lea	edx, [esi+5]
	jne	.L1382
	test	al, al
	jns	.L866
	fld1
	fadd	QWORD PTR [ebx]
	fstp	QWORD PTR [ebx]
	jmp	.L866
	.p2align 4,,10
	.p2align 3
.L841:
	test	al, 1
	je	.L849
	movzx	ebx, BYTE PTR [esi]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+5]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 64
	mov	DWORD PTR [ebp-44], edx
	jne	.L1383
	test	al, al
	jns	.L851
	fld	QWORD PTR [esi]
	fld	QWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1309
	.p2align 4,,10
	.p2align 3
.L828:
	test	al, 1
	je	.L836
	movzx	ebx, BYTE PTR [esi]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+5]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 64
	mov	DWORD PTR [ebp-44], edx
	jne	.L1384
	test	al, al
	jns	.L838
	fld	QWORD PTR [esi]
	fld	QWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1305
	.p2align 4,,10
	.p2align 3
.L789:
	test	al, 1
	je	.L797
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+9]
	mov	DWORD PTR [ebp-44], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L798
	fld	DWORD PTR [ebp-48]
	mov	esi, ecx
	fdivr	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L815:
	test	al, 1
	je	.L823
	movzx	ebx, BYTE PTR [esi]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+5]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 64
	mov	DWORD PTR [ebp-44], edx
	jne	.L1385
	test	al, al
	jns	.L825
	fld	QWORD PTR [esi]
	fld	QWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1301
	.p2align 4,,10
	.p2align 3
.L763:
	test	al, 1
	je	.L771
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+9]
	mov	DWORD PTR [ebp-44], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 64
	jne	.L1386
	test	al, al
	jns	.L1294
	fld	QWORD PTR [ebp-48]
	mov	esi, ecx
	fsubr	QWORD PTR [edx]
	fstp	QWORD PTR [edx]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L802:
	test	al, 1
	je	.L810
	movzx	ebx, BYTE PTR [esi]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+5]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 64
	mov	DWORD PTR [ebp-44], edx
	je	.L811
	fld	DWORD PTR [ebp-48]
	fld	DWORD PTR [esi]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1298
	.p2align 4,,10
	.p2align 3
.L776:
	test	al, 1
	je	.L784
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+9]
	mov	DWORD PTR [ebp-44], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 64
	jne	.L1387
	test	al, al
	jns	.L1295
	fld	QWORD PTR [ebp-48]
	mov	esi, ecx
	fmul	QWORD PTR [edx]
	fstp	QWORD PTR [edx]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L750:
	test	al, 1
	je	.L758
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+9]
	mov	DWORD PTR [ebp-44], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L759
	fld	DWORD PTR [ebp-48]
	mov	esi, ecx
	fadd	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L1363:
	add	DWORD PTR [edi+ecx*8], 1
	lea	esi, [edx+2]
	adc	DWORD PTR [edi+4+ecx*8], 0
	movzx	eax, WORD PTR [edx]
	movzx	edx, al
	cmp	dl, 46
	jbe	.L1271
	jmp	.L342
	.p2align 4,,10
	.p2align 3
.L666:
	test	al, 1
	je	.L673
	mov	ebx, DWORD PTR [esi+5]
	mov	edx, DWORD PTR [esi+1]
	lea	ecx, [esi+13]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1388
	test	al, 32
	je	.L676
	cmp	WORD PTR [esi], dx
.L1283:
	movzx	ebx, BYTE PTR [edi+212]
	sete	dl
	and	ebx, -2
	or	ebx, edx
	mov	BYTE PTR [edi+212], bl
.L675:
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L354:
	test	al, 2
	je	.L360
	movzx	ebx, BYTE PTR [esi]
	mov	eax, DWORD PTR [esi+1]
	add	esi, 9
	mov	edx, DWORD PTR [esi-4]
	mov	DWORD PTR [edi+ebx*8], eax
	mov	DWORD PTR [edi+4+ebx*8], edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L614:
	test	al, 1
	je	.L627
	movzx	ebx, BYTE PTR [esi]
	mov	edx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	DWORD PTR [ebp-48], edx
	lea	edx, [esi+13]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1389
	test	al, 32
	je	.L630
	cmp	WORD PTR [esi], cx
.L1279:
	movzx	eax, BYTE PTR [edi+212]
	setb	bl
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
.L629:
	mov	esi, edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L378:
	test	al, 1
	je	.L385
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+9]
	mov	DWORD PTR [ebp-44], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1390
	test	al, 32
	je	.L388
	movzx	eax, WORD PTR [ebp-48]
	add	WORD PTR [edx], ax
.L387:
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L1370:
	lea	edx, [esi+8]
	mov	esi, DWORD PTR [esi]
	add	esi, edx
	test	BYTE PTR [edi+212], 1
	cmovne	esi, edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L442:
	test	al, 1
	je	.L449
	lea	edx, [esi+13]
	mov	ecx, DWORD PTR [esi+1]
	mov	ebx, DWORD PTR [esi+5]
	mov	DWORD PTR [ebp-48], edx
	movzx	edx, BYTE PTR [esi]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+edx*8]
	test	al, 16
	jne	.L1391
	test	al, 32
	je	.L452
	movzx	eax, WORD PTR [esi]
	xor	edx, edx
	div	cx
	mov	WORD PTR [esi], dx
.L451:
	mov	esi, DWORD PTR [ebp-48]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L1367:
	neg	DWORD PTR [edi+ecx*8]
	lea	esi, [edx+2]
	adc	DWORD PTR [edi+4+ecx*8], 0
	neg	DWORD PTR [edi+4+ecx*8]
	movzx	eax, WORD PTR [edx]
	movzx	edx, al
	cmp	dl, 46
	jbe	.L1271
	jmp	.L342
	.p2align 4,,10
	.p2align 3
.L1358:
	mov	ebx, DWORD PTR [edi+176]
	movzx	ecx, BYTE PTR [esi]
	add	esi, 1
	lea	edx, [ebx+8]
	mov	eax, DWORD PTR [ebx]
	mov	DWORD PTR [edi+176], edx
	mov	edx, DWORD PTR [ebx+4]
	mov	DWORD PTR [edi+ecx*8], eax
	mov	DWORD PTR [edi+4+ecx*8], edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L1357:
	mov	ebx, DWORD PTR [edi+176]
	mov	ecx, DWORD PTR [esi]
	add	esi, 8
	mov	edx, DWORD PTR [esi-4]
	lea	eax, [ebx-8]
	mov	DWORD PTR [ebx-8], ecx
	mov	DWORD PTR [ebx-4], edx
	mov	DWORD PTR [edi+176], eax
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L719:
	test	al, 2
	je	.L721
	movzx	ecx, BYTE PTR [esi]
	lea	ebx, [esi+1]
	mov	DWORD PTR [ebp-60], ebx
	mov	eax, DWORD PTR [edi+ecx*8]
	add	eax, -1
	jmp	.L720
	.p2align 4,,10
	.p2align 3
.L729:
	test	al, 2
	je	.L731
	movzx	ebx, BYTE PTR [esi+4]
	add	esi, 5
	mov	edx, DWORD PTR [edi+ebx*8]
	not	edx
	mov	DWORD PTR [ebp-48], edx
	jmp	.L730
	.p2align 4,,10
	.p2align 3
.L1371:
	lea	edx, [esi+8]
	mov	esi, DWORD PTR [esi]
	add	esi, edx
	test	BYTE PTR [edi+212], 1
	cmove	esi, edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L1369:
	mov	ebx, DWORD PTR [esi]
	lea	esi, [esi+8+ebx]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L682:
	test	al, 1
	je	.L689
	mov	ebx, DWORD PTR [esi+5]
	mov	edx, DWORD PTR [esi+1]
	lea	ecx, [esi+13]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1392
	test	al, 32
	je	.L692
	cmp	WORD PTR [esi], dx
.L1287:
	movzx	ebx, BYTE PTR [edi+212]
	setne	dl
	and	ebx, -2
	or	ebx, edx
	mov	BYTE PTR [edi+212], bl
.L691:
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L640:
	test	al, 1
	je	.L653
	movzx	ebx, BYTE PTR [esi]
	mov	edx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	DWORD PTR [ebp-48], edx
	lea	edx, [esi+13]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	je	.L654
	cmp	BYTE PTR [esi], cl
.L1281:
	movzx	eax, BYTE PTR [edi+212]
	seta	bl
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
.L655:
	mov	esi, edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L394:
	test	al, 1
	je	.L401
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+9]
	mov	DWORD PTR [ebp-44], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	DWORD PTR [ebp-48], ecx
	lea	ecx, [esi+13]
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1393
	test	al, 32
	je	.L404
	movzx	eax, WORD PTR [ebp-48]
	sub	WORD PTR [edx], ax
.L403:
	mov	esi, ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L588:
	test	al, 1
	je	.L601
	movzx	ebx, BYTE PTR [esi]
	mov	edx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	DWORD PTR [ebp-48], edx
	lea	edx, [esi+13]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1394
	test	al, 32
	je	.L604
	cmp	WORD PTR [esi], cx
.L1277:
	movzx	eax, BYTE PTR [edi+212]
	setg	bl
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
.L603:
	mov	esi, edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L1365:
	add	DWORD PTR [edi+ecx*8], -1
	lea	esi, [edx+2]
	adc	DWORD PTR [edi+4+ecx*8], -1
	movzx	eax, WORD PTR [edx]
	movzx	edx, al
	cmp	dl, 46
	jbe	.L1271
	jmp	.L342
	.p2align 4,,10
	.p2align 3
.L562:
	test	al, 1
	je	.L575
	movzx	ebx, BYTE PTR [esi]
	mov	edx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	mov	DWORD PTR [ebp-48], edx
	lea	edx, [esi+13]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	je	.L576
	cmp	BYTE PTR [esi], cl
.L1275:
	movzx	eax, BYTE PTR [edi+212]
	setl	bl
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
.L577:
	mov	esi, edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L1361:
	lea	ecx, [edi+ecx*8]
	lea	esi, [edx+2]
	not	DWORD PTR [ecx]
	not	DWORD PTR [ecx+4]
	movzx	eax, WORD PTR [edx]
	movzx	edx, al
	cmp	dl, 46
	jbe	.L1271
	jmp	.L342
	.p2align 4,,10
	.p2align 3
.L490:
	test	al, 1
	je	.L497
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	lea	edx, [esi+13]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1395
	test	al, 32
	je	.L500
	xor	WORD PTR [esi], cx
.L499:
	mov	esi, edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L528:
	test	al, 1
	je	.L535
	lea	edx, [esi+13]
	mov	ebx, DWORD PTR [esi+9]
	mov	ecx, DWORD PTR [esi+1]
	mov	DWORD PTR [ebp-48], edx
	movzx	edx, BYTE PTR [esi]
	add	ebx, DWORD PTR [edi+edx*8]
	test	al, 16
	je	.L536
	movzx	edx, BYTE PTR [ebx]
	sar	edx, cl
	mov	BYTE PTR [ebx], dl
.L537:
	mov	esi, DWORD PTR [ebp-48]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L362:
	test	al, 1
	je	.L369
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	lea	edx, [esi+13]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1396
	test	al, 32
	je	.L372
	mov	WORD PTR [esi], cx
.L371:
	mov	esi, edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L426:
	test	al, 1
	je	.L433
	lea	edx, [esi+13]
	mov	ecx, DWORD PTR [esi+1]
	mov	ebx, DWORD PTR [esi+5]
	mov	DWORD PTR [ebp-48], edx
	movzx	edx, BYTE PTR [esi]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+edx*8]
	test	al, 16
	jne	.L1397
	test	al, 32
	je	.L436
	movzx	eax, WORD PTR [esi]
	xor	edx, edx
	div	cx
	mov	WORD PTR [esi], ax
.L435:
	mov	esi, DWORD PTR [ebp-48]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L474:
	test	al, 1
	je	.L481
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	lea	edx, [esi+13]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1398
	test	al, 32
	je	.L484
	or	WORD PTR [esi], cx
.L483:
	mov	esi, edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L410:
	test	al, 1
	je	.L417
	lea	ebx, [esi+13]
	mov	ecx, DWORD PTR [esi+1]
	mov	edx, DWORD PTR [esi+5]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	DWORD PTR [ebp-52], ecx
	mov	ecx, DWORD PTR [esi+9]
	add	ecx, DWORD PTR [edi+ebx*8]
	test	al, 16
	je	.L418
	movzx	eax, BYTE PTR [ebp-52]
	mul	BYTE PTR [ecx]
	mov	BYTE PTR [ecx], al
.L419:
	mov	esi, DWORD PTR [ebp-48]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L512:
	test	al, 1
	je	.L519
	lea	edx, [esi+13]
	mov	ebx, DWORD PTR [esi+9]
	mov	ecx, DWORD PTR [esi+1]
	mov	DWORD PTR [ebp-48], edx
	movzx	edx, BYTE PTR [esi]
	add	ebx, DWORD PTR [edi+edx*8]
	test	al, 16
	je	.L520
	movzx	edx, BYTE PTR [ebx]
	sal	edx, cl
	mov	BYTE PTR [ebx], dl
.L521:
	mov	esi, DWORD PTR [ebp-48]
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L458:
	test	al, 1
	je	.L465
	mov	ebx, DWORD PTR [esi+5]
	mov	ecx, DWORD PTR [esi+1]
	lea	edx, [esi+13]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, BYTE PTR [esi]
	mov	esi, DWORD PTR [esi+9]
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	jne	.L1399
	test	al, 32
	je	.L468
	and	WORD PTR [esi], cx
.L467:
	mov	esi, edx
	jmp	.L845
.L1261:
	mov	esi, DWORD PTR [ebp-68]
	mov	edi, DWORD PTR [ebp-52]
.L733:
	mov	ecx, DWORD PTR [ecx+eax]
	test	ecx, ecx
	je	.L338
	mov	ebx, DWORD PTR [ebp-64]
	mov	DWORD PTR [edi], 0
	mov	DWORD PTR [edi+4], 0
	cmp	ebx, 8
	ja	.L737
	lea	eax, [edi+112]
	push	eax
	push	ebx
	push	edi
	push	edi
	call	ecx
	add	esp, 16
	jmp	.L845
.L1265:
	mov	edi, DWORD PTR [ebp-48]
.L723:
	add	ecx, eax
	je	.L722
	mov	eax, DWORD PTR [edi+176]
	mov	esi, DWORD PTR [ebp-60]
	mov	edx, DWORD PTR [edi+184]
	mov	DWORD PTR [eax-8], esi
	mov	esi, DWORD PTR [edi+188]
	lea	ebx, [eax-16]
	mov	DWORD PTR [eax-16], edx
	mov	DWORD PTR [edi+176], ebx
	mov	ebx, DWORD PTR [edi+180]
	mov	DWORD PTR [eax-12], esi
	mov	eax, DWORD PTR [edi+176]
	lea	esi, [ecx+2]
	mov	DWORD PTR [edi+188], ebx
	mov	DWORD PTR [edi+184], eax
	movzx	eax, WORD PTR [ecx]
	movzx	edx, al
	cmp	dl, 46
	jbe	.L1271
	jmp	.L342
	.p2align 4,,10
	.p2align 3
.L737:
	mov	eax, DWORD PTR [ebp-64]
	mov	DWORD PTR [ebp-68], ecx
	mov	DWORD PTR [ebp-52], esp
	lea	ecx, [0+eax*8]
	lea	edx, [ecx+18]
	sub	ecx, 64
	and	edx, -16
	sub	esp, edx
	mov	edx, DWORD PTR [edi+112]
	lea	eax, [esp+3]
	sub	esp, 4
	shr	eax, 2
	lea	ebx, [0+eax*4]
	mov	DWORD PTR [0+eax*4], edx
	mov	edx, DWORD PTR [edi+120]
	mov	DWORD PTR [ebp-48], ebx
	mov	ebx, DWORD PTR [edi+116]
	mov	DWORD PTR [8+eax*4], edx
	mov	edx, DWORD PTR [edi+128]
	mov	DWORD PTR [4+eax*4], ebx
	mov	ebx, DWORD PTR [edi+124]
	mov	DWORD PTR [16+eax*4], edx
	mov	edx, DWORD PTR [edi+136]
	mov	DWORD PTR [12+eax*4], ebx
	mov	ebx, DWORD PTR [edi+132]
	mov	DWORD PTR [24+eax*4], edx
	mov	edx, DWORD PTR [edi+144]
	mov	DWORD PTR [20+eax*4], ebx
	mov	ebx, DWORD PTR [edi+140]
	mov	DWORD PTR [32+eax*4], edx
	mov	edx, DWORD PTR [edi+152]
	mov	DWORD PTR [28+eax*4], ebx
	mov	ebx, DWORD PTR [edi+148]
	mov	DWORD PTR [40+eax*4], edx
	mov	edx, DWORD PTR [edi+160]
	mov	DWORD PTR [36+eax*4], ebx
	mov	ebx, DWORD PTR [edi+156]
	mov	DWORD PTR [48+eax*4], edx
	mov	DWORD PTR [44+eax*4], ebx
	mov	ebx, DWORD PTR [edi+164]
	mov	DWORD PTR [52+eax*4], ebx
	mov	edx, DWORD PTR [edi+168]
	mov	ebx, DWORD PTR [edi+172]
	mov	DWORD PTR [56+eax*4], edx
	mov	DWORD PTR [60+eax*4], ebx
	mov	eax, DWORD PTR [ebp-48]
	mov	ebx, DWORD PTR [edi+176]
	push	ecx
	mov	DWORD PTR [ebp-60], ecx
	lea	edx, [eax+64]
	push	ebx
	push	edx
	call	memcpy
	mov	ecx, DWORD PTR [ebp-60]
	add	ebx, ecx
	mov	DWORD PTR [edi+176], ebx
	push	DWORD PTR [ebp-48]
	mov	ebx, DWORD PTR [ebp-68]
	push	DWORD PTR [ebp-64]
	push	edi
	push	edi
	call	ebx
	mov	esp, DWORD PTR [ebp-52]
	jmp	.L845
.L433:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ebx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, dl
	add	ebx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L438
	movzx	esi, dh
	movzx	eax, BYTE PTR [ebx]
	div	BYTE PTR [edi+esi*8]
	mov	esi, ecx
	mov	BYTE PTR [ebx], al
	jmp	.L845
.L865:
	test	al, al
	jns	.L866
	lea	ecx, [edi+ecx*8]
	fld	QWORD PTR [ecx]
	fchs
	fstp	QWORD PTR [ecx]
	jmp	.L866
.L673:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dl
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L678
	movzx	esi, dh
	movzx	ecx, BYTE PTR [ecx]
	cmp	BYTE PTR [edi+esi*8], cl
	jmp	.L1285
.L707:
	test	al, 4
	je	.L845
	movzx	ebx, BYTE PTR [esi]
	lea	ecx, [esi+5]
	mov	edx, DWORD PTR [esi+1]
	mov	esi, ecx
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 16
	je	.L708
	test	BYTE PTR [edi+212], 1
	jne	.L845
	movsx	ecx, BYTE PTR [edx]
	add	esi, ecx
	jmp	.L845
.L443:
	test	al, 2
	jne	.L1400
	test	al, 4
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bh
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	mov	esi, edx
	je	.L446
	movzx	ebx, bl
	lea	edx, [edi+ebx*8]
	movzx	eax, BYTE PTR [edx]
	div	BYTE PTR [esi]
	mov	esi, ecx
	movzx	ebx, ah
	mov	BYTE PTR [edx], bl
	jmp	.L845
.L615:
	test	al, 2
	je	.L619
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+2]
	movzx	edx, cl
	movzx	eax, ch
	mov	ecx, 1
	mov	esi, DWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edi+4+eax*8]
	cmp	DWORD PTR [edi+4+edx*8], esi
	jb	.L620
	ja	.L621
	mov	edx, DWORD PTR [ebp-48]
	cmp	edx, DWORD PTR [edi+eax*8]
	jnb	.L621
.L620:
	movzx	eax, BYTE PTR [edi+212]
	mov	esi, ebx
	and	eax, -2
	or	ecx, eax
	mov	BYTE PTR [edi+212], cl
	jmp	.L845
.L491:
	test	al, 2
	jne	.L1401
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L494
	movzx	esi, dl
	movzx	eax, BYTE PTR [ecx]
	xor	BYTE PTR [edi+esi*8], al
	mov	esi, ebx
	jmp	.L845
.L379:
	test	al, 2
	jne	.L1402
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ebx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, dh
	add	ebx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L382
	movzx	esi, dl
	movzx	ebx, BYTE PTR [ebx]
	add	BYTE PTR [edi+esi*8], bl
	mov	esi, ecx
	jmp	.L845
.L411:
	test	al, 2
	jne	.L1403
	test	al, 4
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bh
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L414
	movzx	ebx, bl
	lea	esi, [edi+ebx*8]
	movzx	eax, BYTE PTR [esi]
	mul	BYTE PTR [edx]
	mov	BYTE PTR [esi], al
	mov	esi, ecx
	jmp	.L845
.L529:
	test	al, 2
	jne	.L1404
	test	al, 4
	je	.L845
	movzx	ecx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, ch
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L532
	movzx	esi, cl
	movzx	ecx, BYTE PTR [edx]
	lea	esi, [edi+esi*8]
	movzx	eax, BYTE PTR [esi]
	sar	eax, cl
	mov	BYTE PTR [esi], al
	mov	esi, ebx
	jmp	.L845
.L575:
	test	al, 2
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bl
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L582
	movzx	eax, bh
	movzx	ebx, BYTE PTR [edi+eax*8]
	cmp	BYTE PTR [edx], bl
	jmp	.L1276
.L449:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ebx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, dl
	add	ebx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L454
	movzx	esi, dh
	movzx	eax, BYTE PTR [ebx]
	div	BYTE PTR [edi+esi*8]
	mov	esi, ecx
	movzx	edx, ah
	mov	BYTE PTR [ebx], dl
	jmp	.L845
.L535:
	test	al, 2
	je	.L845
	movzx	ecx, WORD PTR [esi]
	lea	edx, [esi+6]
	mov	esi, DWORD PTR [esi+2]
	movzx	ebx, cl
	add	esi, DWORD PTR [edi+ebx*8]
	test	al, 16
	mov	ebx, esi
	je	.L540
	movzx	eax, BYTE PTR [esi]
	movzx	esi, ch
	movzx	ecx, BYTE PTR [edi+esi*8]
	mov	esi, edx
	sar	eax, cl
	mov	BYTE PTR [ebx], al
	jmp	.L845
.L475:
	test	al, 2
	jne	.L1405
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L478
	movzx	esi, dl
	movzx	eax, BYTE PTR [ecx]
	or	BYTE PTR [edi+esi*8], al
	mov	esi, ebx
	jmp	.L845
.L369:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dl
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L374
	movzx	edx, dh
	mov	esi, ebx
	movzx	eax, BYTE PTR [edi+edx*8]
	mov	BYTE PTR [ecx], al
	jmp	.L845
.L860:
	test	al, al
	jns	.L866
	lea	esi, [edi+ecx*8]
	fld1
	fsubr	QWORD PTR [esi]
	fstp	QWORD PTR [esi]
	jmp	.L866
.L855:
	test	al, al
	jns	.L866
	lea	esi, [edi+ecx*8]
	fld1
	fadd	QWORD PTR [esi]
	fstp	QWORD PTR [esi]
	jmp	.L866
.L777:
	test	al, 2
	jne	.L1406
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 64
	je	.L783
	movzx	esi, dl
	lea	eax, [edi+esi*8]
	mov	esi, ebx
	fld	DWORD PTR [eax]
	fmul	DWORD PTR [ecx]
	fstp	DWORD PTR [eax]
	jmp	.L845
.L764:
	test	al, 2
	jne	.L1407
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 64
	je	.L770
	movzx	esi, dl
	lea	eax, [edi+esi*8]
	mov	esi, ebx
	fld	DWORD PTR [eax]
	fsub	DWORD PTR [ecx]
	fstp	DWORD PTR [eax]
	jmp	.L845
.L803:
	test	al, 2
	jne	.L1408
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 64
	je	.L809
	fld	DWORD PTR [ecx]
	movzx	edx, dl
	movzx	eax, BYTE PTR [edi+212]
	mov	esi, ebx
	fld	DWORD PTR [edi+edx*8]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	seta	cl
	and	eax, -2
	or	eax, ecx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L797:
	test	al, 2
	je	.L845
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+6]
	mov	edx, DWORD PTR [esi+2]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, cl
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L800
	fld	DWORD PTR [edx]
	movzx	esi, ch
	fdiv	DWORD PTR [edi+esi*8]
	mov	esi, DWORD PTR [ebp-48]
	fstp	DWORD PTR [edx]
	jmp	.L845
.L790:
	test	al, 2
	jne	.L1409
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 64
	je	.L796
	movzx	esi, dl
	lea	eax, [edi+esi*8]
	mov	esi, ebx
	fld	DWORD PTR [eax]
	fdiv	DWORD PTR [ecx]
	fstp	DWORD PTR [eax]
	jmp	.L845
.L771:
	test	al, 2
	je	.L845
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+6]
	mov	edx, DWORD PTR [esi+2]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, cl
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L774
	fld	DWORD PTR [edx]
	movzx	esi, ch
	fsub	DWORD PTR [edi+esi*8]
	mov	esi, DWORD PTR [ebp-48]
	fstp	DWORD PTR [edx]
	jmp	.L845
.L849:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	lea	ecx, [esi+6]
	mov	DWORD PTR [ebp-48], ecx
	mov	ecx, DWORD PTR [esi+2]
	movzx	ebx, dl
	add	ecx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L852
	movzx	esi, dh
	movzx	eax, BYTE PTR [edi+212]
	fld	DWORD PTR [edi+esi*8]
	mov	esi, DWORD PTR [ebp-48]
	fld	DWORD PTR [ecx]
	fcomip	st, st(1)
	fstp	st(0)
	setne	bl
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L836:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	lea	ecx, [esi+6]
	mov	DWORD PTR [ebp-48], ecx
	mov	ecx, DWORD PTR [esi+2]
	movzx	ebx, dl
	add	ecx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L839
	movzx	esi, dh
	movzx	eax, BYTE PTR [edi+212]
	fld	DWORD PTR [edi+esi*8]
	mov	esi, DWORD PTR [ebp-48]
	fld	DWORD PTR [ecx]
	fcomip	st, st(1)
	fstp	st(0)
	sete	bl
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L842:
	test	al, 2
	je	.L846
	test	al, 64
	lea	ecx, [esi+2]
	movzx	ebx, WORD PTR [esi]
	je	.L847
	movzx	esi, bl
	movzx	eax, bh
	fld	DWORD PTR [edi+esi*8]
	fld	DWORD PTR [edi+eax*8]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
.L1309:
	movzx	eax, BYTE PTR [edi+212]
	setne	dl
	and	eax, -2
	or	eax, edx
	mov	BYTE PTR [edi+212], al
.L851:
	mov	esi, ecx
	jmp	.L845
.L810:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	lea	ecx, [esi+6]
	mov	DWORD PTR [ebp-48], ecx
	mov	ecx, DWORD PTR [esi+2]
	movzx	ebx, dl
	add	ecx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L813
	movzx	esi, dh
	movzx	ebx, BYTE PTR [edi+212]
	fld	DWORD PTR [edi+esi*8]
	mov	esi, DWORD PTR [ebp-48]
	fld	DWORD PTR [ecx]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	seta	dl
	and	ebx, -2
	or	ebx, edx
	mov	BYTE PTR [edi+212], bl
	jmp	.L845
.L829:
	test	al, 2
	je	.L833
	test	al, 64
	lea	ecx, [esi+2]
	movzx	ebx, WORD PTR [esi]
	je	.L834
	movzx	esi, bl
	movzx	eax, bh
	fld	DWORD PTR [edi+esi*8]
	fld	DWORD PTR [edi+eax*8]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
.L1305:
	movzx	eax, BYTE PTR [edi+212]
	sete	dl
	and	eax, -2
	or	eax, edx
	mov	BYTE PTR [edi+212], al
.L838:
	mov	esi, ecx
	jmp	.L845
.L823:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	lea	ecx, [esi+6]
	mov	DWORD PTR [ebp-48], ecx
	mov	ecx, DWORD PTR [esi+2]
	movzx	ebx, dl
	add	ecx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L826
	fld	DWORD PTR [ecx]
	movzx	esi, dh
	movzx	eax, BYTE PTR [edi+212]
	fld	DWORD PTR [edi+esi*8]
	fxch	st(1)
	mov	esi, DWORD PTR [ebp-48]
	fcomip	st, st(1)
	fstp	st(0)
	seta	bl
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L816:
	test	al, 2
	je	.L820
	test	al, 64
	lea	ecx, [esi+2]
	movzx	ebx, WORD PTR [esi]
	je	.L821
	movzx	esi, bl
	movzx	ebx, bh
	fld	DWORD PTR [edi+esi*8]
	fld	DWORD PTR [edi+ebx*8]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
.L1301:
	movzx	eax, BYTE PTR [edi+212]
	seta	dl
	and	eax, -2
	or	eax, edx
	mov	BYTE PTR [edi+212], al
.L825:
	mov	esi, ecx
	jmp	.L845
.L784:
	test	al, 2
	je	.L845
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+6]
	mov	edx, DWORD PTR [esi+2]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, cl
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L787
	fld	DWORD PTR [edx]
	movzx	esi, ch
	fmul	DWORD PTR [edi+esi*8]
	mov	esi, DWORD PTR [ebp-48]
	fstp	DWORD PTR [edx]
	jmp	.L845
.L627:
	test	al, 2
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bl
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L634
	movzx	eax, bh
	movzx	ebx, BYTE PTR [edi+eax*8]
	cmp	BYTE PTR [edx], bl
.L1280:
	movzx	eax, BYTE PTR [edi+212]
	setb	bl
	mov	esi, ecx
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L401:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dl
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L406
	movzx	edx, dh
	mov	esi, ebx
	movzx	eax, BYTE PTR [edi+edx*8]
	sub	BYTE PTR [ecx], al
	jmp	.L845
.L601:
	test	al, 2
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bl
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L608
	movzx	eax, bh
	movzx	ebx, BYTE PTR [edi+eax*8]
	cmp	BYTE PTR [edx], bl
	jmp	.L1278
.L519:
	test	al, 2
	je	.L845
	movzx	ecx, WORD PTR [esi]
	mov	ebx, DWORD PTR [esi+2]
	lea	edx, [esi+6]
	movzx	esi, cl
	add	ebx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L524
	movzx	esi, ch
	movzx	eax, BYTE PTR [ebx]
	movzx	ecx, BYTE PTR [edi+esi*8]
	mov	esi, edx
	sal	eax, cl
	mov	BYTE PTR [ebx], al
	jmp	.L845
.L363:
	test	al, 2
	jne	.L1410
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L366
	movzx	ecx, BYTE PTR [ecx]
	movzx	esi, dl
	mov	BYTE PTR [edi+esi*8], cl
	mov	esi, ebx
	jmp	.L845
.L513:
	test	al, 2
	jne	.L1411
	test	al, 4
	je	.L845
	movzx	ecx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, ch
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L516
	movzx	esi, cl
	movzx	ecx, BYTE PTR [edx]
	lea	esi, [edi+esi*8]
	movzx	eax, BYTE PTR [esi]
	sal	eax, cl
	mov	BYTE PTR [esi], al
	mov	esi, ebx
	jmp	.L845
.L395:
	test	al, 2
	jne	.L1412
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ebx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, dh
	add	ebx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L398
	movzx	esi, dl
	movzx	ebx, BYTE PTR [ebx]
	sub	BYTE PTR [edi+esi*8], bl
	mov	esi, ecx
	jmp	.L845
.L459:
	test	al, 2
	jne	.L1413
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L462
	movzx	esi, dl
	movzx	eax, BYTE PTR [ecx]
	and	BYTE PTR [edi+esi*8], al
	mov	esi, ebx
	jmp	.L845
.L683:
	test	al, 2
	jne	.L1414
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L686
	movzx	edx, dl
	movzx	eax, BYTE PTR [ecx]
	cmp	BYTE PTR [edi+edx*8], al
	jmp	.L1289
.L758:
	test	al, 2
	je	.L845
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+6]
	mov	edx, DWORD PTR [esi+2]
	mov	DWORD PTR [ebp-48], ebx
	movzx	ebx, cl
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 64
	je	.L761
	fld	DWORD PTR [edx]
	movzx	esi, ch
	fadd	DWORD PTR [edi+esi*8]
	mov	esi, DWORD PTR [ebp-48]
	fstp	DWORD PTR [edx]
	jmp	.L845
.L360:
	test	al, 4
	je	.L845
	movzx	ecx, WORD PTR [esi]
	mov	eax, DWORD PTR [esi+2]
	add	esi, 6
	movzx	edx, ch
	movzx	ebx, cl
	add	eax, DWORD PTR [edi+edx*8]
	mov	DWORD PTR [edi+ebx*8], eax
	jmp	.L845
.L721:
	test	al, 4
	je	.L722
	movzx	ecx, BYTE PTR [esi]
	test	al, al
	mov	edx, DWORD PTR [esi+1]
	mov	ebx, DWORD PTR [edi+ecx*8]
	js	.L1415
.L722:
	mov	DWORD PTR [edi+208], 2
	mov	eax, -1
	jmp	.L337
	.p2align 4,,10
	.p2align 3
.L751:
	test	al, 2
	jne	.L1416
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 64
	je	.L757
	movzx	esi, dl
	lea	eax, [edi+esi*8]
	mov	esi, ebx
	fld	DWORD PTR [eax]
	fadd	DWORD PTR [ecx]
	fstp	DWORD PTR [eax]
	jmp	.L845
.L497:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dl
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L502
	movzx	edx, dh
	mov	esi, ebx
	movzx	eax, BYTE PTR [edi+edx*8]
	xor	BYTE PTR [ecx], al
	jmp	.L845
.L653:
	test	al, 2
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bl
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L660
	movzx	eax, bh
	movzx	ebx, BYTE PTR [edi+eax*8]
	cmp	BYTE PTR [edx], bl
	jmp	.L1282
.L417:
	test	al, 2
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	edx, [esi+6]
	movzx	esi, bl
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L422
	movzx	esi, bh
	movzx	eax, BYTE PTR [ecx]
	mul	BYTE PTR [edi+esi*8]
	mov	esi, edx
	mov	BYTE PTR [ecx], al
	jmp	.L845
.L385:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dl
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L390
	movzx	edx, dh
	mov	esi, ebx
	movzx	eax, BYTE PTR [edi+edx*8]
	add	BYTE PTR [ecx], al
	jmp	.L845
.L689:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dl
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L694
	movzx	esi, dh
	movzx	ecx, BYTE PTR [ecx]
	cmp	BYTE PTR [edi+esi*8], cl
.L1289:
	movzx	edx, BYTE PTR [edi+212]
	setne	cl
	mov	esi, ebx
	and	edx, -2
	or	edx, ecx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
.L427:
	test	al, 2
	jne	.L1417
	test	al, 4
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bh
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	mov	esi, edx
	je	.L430
	movzx	ebx, bl
	lea	edx, [edi+ebx*8]
	movzx	eax, BYTE PTR [edx]
	div	BYTE PTR [esi]
	mov	esi, ecx
	mov	BYTE PTR [edx], al
	jmp	.L845
.L700:
	test	al, 4
	je	.L845
	movzx	ebx, BYTE PTR [esi]
	mov	edx, DWORD PTR [esi+1]
	lea	ecx, [esi+5]
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 16
	je	.L701
	movsx	esi, BYTE PTR [edx]
	add	esi, ecx
	jmp	.L845
.L731:
	test	al, 4
	je	.L338
	movzx	ebx, BYTE PTR [esi+4]
	mov	ecx, DWORD PTR [esi+5]
	add	esi, 9
	mov	edx, DWORD PTR [edi+ebx*8]
	mov	eax, DWORD PTR [edx+ecx]
	not	eax
	mov	DWORD PTR [ebp-48], eax
	jmp	.L730
.L714:
	test	al, 4
	je	.L845
	movzx	ebx, BYTE PTR [esi]
	lea	ecx, [esi+5]
	mov	edx, DWORD PTR [esi+1]
	mov	esi, ecx
	add	edx, DWORD PTR [edi+ebx*8]
	test	al, 16
	je	.L715
	test	BYTE PTR [edi+212], 1
	je	.L845
	movsx	ecx, BYTE PTR [edx]
	add	esi, ecx
	jmp	.L845
.L667:
	test	al, 2
	jne	.L1418
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L670
	movzx	edx, dl
	movzx	eax, BYTE PTR [ecx]
	cmp	BYTE PTR [edi+edx*8], al
.L1285:
	movzx	edx, BYTE PTR [edi+212]
	sete	cl
	mov	esi, ebx
	and	edx, -2
	or	edx, ecx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
.L481:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dl
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L486
	movzx	edx, dh
	mov	esi, ebx
	movzx	eax, BYTE PTR [edi+edx*8]
	or	BYTE PTR [ecx], al
	jmp	.L845
.L563:
	test	al, 2
	jne	.L1419
	test	al, 4
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bh
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L570
	movzx	eax, bl
	movzx	edx, BYTE PTR [edx]
	cmp	BYTE PTR [edi+eax*8], dl
.L1276:
	movzx	eax, BYTE PTR [edi+212]
	setl	bl
	mov	esi, ecx
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L589:
	test	al, 2
	jne	.L1420
	test	al, 4
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bh
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L596
	movzx	eax, bl
	movzx	edx, BYTE PTR [edx]
	cmp	BYTE PTR [edi+eax*8], dl
.L1278:
	movzx	eax, BYTE PTR [edi+212]
	setg	bl
	mov	esi, ecx
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L348:
	test	al, 4
	je	.L845
	movzx	ecx, BYTE PTR [esi]
	mov	ebx, DWORD PTR [esi+1]
	add	esi, 5
	add	ebx, DWORD PTR [edi+ecx*8]
	mov	ecx, DWORD PTR [edi+176]
	lea	edx, [ecx-8]
	mov	eax, DWORD PTR [ebx]
	mov	DWORD PTR [edi+176], edx
	mov	edx, DWORD PTR [ebx+4]
	mov	DWORD PTR [ecx-8], eax
	mov	DWORD PTR [ecx-4], edx
	jmp	.L845
.L641:
	test	al, 2
	jne	.L1421
	test	al, 4
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bh
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L648
	movzx	eax, bl
	movzx	edx, BYTE PTR [edx]
	cmp	BYTE PTR [edi+eax*8], dl
.L1282:
	movzx	eax, BYTE PTR [edi+212]
	seta	bl
	mov	esi, ecx
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L465:
	test	al, 2
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dl
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L470
	movzx	edx, dh
	mov	esi, ebx
	movzx	eax, BYTE PTR [edi+edx*8]
	and	BYTE PTR [ecx], al
	jmp	.L845
.L1359:
	mov	edi, DWORD PTR [ebp-48]
	xor	eax, eax
	jmp	.L355
.L1372:
	mov	DWORD PTR [ebp-68], esi
	xor	ebx, ebx
	mov	DWORD PTR [ebp-52], edi
	mov	esi, DWORD PTR [ebp-48]
	jmp	.L734
	.p2align 4,,10
	.p2align 3
.L341:
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L1421:
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+2]
	movzx	edx, cl
	movzx	eax, ch
	mov	ecx, 1
	mov	esi, DWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edi+4+eax*8]
	cmp	DWORD PTR [edi+4+edx*8], esi
	ja	.L646
	jb	.L647
	mov	edx, DWORD PTR [ebp-48]
	cmp	edx, DWORD PTR [edi+eax*8]
	jbe	.L647
.L646:
	movzx	eax, BYTE PTR [edi+212]
	mov	esi, ebx
	and	eax, -2
	or	ecx, eax
	mov	BYTE PTR [edi+212], cl
	jmp	.L845
.L1418:
	movzx	ebx, WORD PTR [esi]
	movzx	eax, bh
	movzx	ecx, bl
	mov	edx, DWORD PTR [edi+ecx*8]
	mov	ebx, DWORD PTR [edi+4+ecx*8]
	xor	edx, DWORD PTR [edi+eax*8]
	xor	ebx, DWORD PTR [edi+4+eax*8]
	movzx	eax, BYTE PTR [edi+212]
	or	edx, ebx
	sete	dl
	and	eax, -2
	add	esi, 2
	or	eax, edx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L1417:
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+2]
	movzx	esi, cl
	movzx	edx, ch
	lea	esi, [edi+esi*8]
	push	DWORD PTR [edi+4+edx*8]
	push	DWORD PTR [edi+edx*8]
	push	DWORD PTR [esi+4]
	push	DWORD PTR [esi]
	call	__udivdi3
	add	esp, 16
	mov	DWORD PTR [esi], eax
	mov	DWORD PTR [esi+4], edx
	mov	esi, ebx
	jmp	.L845
.L1419:
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+2]
	movzx	edx, cl
	movzx	eax, ch
	mov	ecx, 1
	mov	esi, DWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edi+4+eax*8]
	cmp	DWORD PTR [edi+4+edx*8], esi
	jl	.L568
	jg	.L569
	mov	edx, DWORD PTR [ebp-48]
	cmp	edx, DWORD PTR [edi+eax*8]
	jnb	.L569
.L568:
	movzx	eax, BYTE PTR [edi+212]
	mov	esi, ebx
	and	eax, -2
	or	ecx, eax
	mov	BYTE PTR [edi+212], cl
	jmp	.L845
.L1420:
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+2]
	movzx	edx, cl
	movzx	eax, ch
	mov	ecx, 1
	mov	esi, DWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edi+4+eax*8]
	cmp	DWORD PTR [edi+4+edx*8], esi
	jg	.L594
	jl	.L595
	mov	edx, DWORD PTR [ebp-48]
	cmp	edx, DWORD PTR [edi+eax*8]
	jbe	.L595
.L594:
	movzx	eax, BYTE PTR [edi+212]
	mov	esi, ebx
	and	eax, -2
	or	ecx, eax
	mov	BYTE PTR [edi+212], cl
	jmp	.L845
.L1416:
	test	al, 64
	lea	ecx, [esi+2]
	movzx	ebx, WORD PTR [esi]
	je	.L756
	movzx	esi, bl
	movzx	ebx, bh
	lea	eax, [edi+esi*8]
	mov	esi, ecx
	fld	DWORD PTR [eax]
	fadd	DWORD PTR [edi+ebx*8]
	fstp	DWORD PTR [eax]
	jmp	.L845
.L833:
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 64
	je	.L835
	movzx	eax, dl
	movzx	edx, BYTE PTR [edi+212]
	mov	esi, ebx
	fld	DWORD PTR [edi+eax*8]
	fld	DWORD PTR [ecx]
	fcomip	st, st(1)
	fstp	st(0)
	sete	cl
	and	edx, -2
	or	edx, ecx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
.L820:
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 64
	je	.L822
	movzx	edx, dl
	movzx	eax, BYTE PTR [edi+212]
	mov	esi, ebx
	fld	DWORD PTR [edi+edx*8]
	fld	DWORD PTR [ecx]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	seta	cl
	and	eax, -2
	or	eax, ecx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L1410:
	movzx	ebx, WORD PTR [esi]
	add	esi, 2
	movzx	eax, bh
	movzx	ecx, bl
	mov	edx, DWORD PTR [edi+4+eax*8]
	mov	ebx, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [edi+4+ecx*8], edx
	mov	DWORD PTR [edi+ecx*8], ebx
	jmp	.L845
.L1411:
	movzx	eax, WORD PTR [esi]
	movzx	ebx, al
	movzx	edx, ah
	lea	ebx, [edi+ebx*8]
	mov	ecx, DWORD PTR [edi+edx*8]
	mov	eax, DWORD PTR [ebx]
	mov	edx, DWORD PTR [ebx+4]
	shld	edx, eax, cl
	sal	eax, cl
	test	cl, 32
	je	.L1425
	mov	edx, eax
	xor	eax, eax
.L1425:
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	add	esi, 2
	jmp	.L845
.L846:
	test	al, 4
	je	.L845
	movzx	edx, WORD PTR [esi]
	mov	ecx, DWORD PTR [esi+2]
	lea	ebx, [esi+6]
	movzx	esi, dh
	add	ecx, DWORD PTR [edi+esi*8]
	test	al, 64
	je	.L848
	movzx	eax, dl
	movzx	edx, BYTE PTR [edi+212]
	mov	esi, ebx
	fld	DWORD PTR [edi+eax*8]
	fld	DWORD PTR [ecx]
	fcomip	st, st(1)
	fstp	st(0)
	setne	cl
	and	edx, -2
	or	edx, ecx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
.L1414:
	movzx	ebx, WORD PTR [esi]
	movzx	eax, bh
	movzx	ecx, bl
	mov	edx, DWORD PTR [edi+ecx*8]
	mov	ebx, DWORD PTR [edi+4+ecx*8]
	xor	edx, DWORD PTR [edi+eax*8]
	xor	ebx, DWORD PTR [edi+4+eax*8]
	movzx	eax, BYTE PTR [edi+212]
	or	edx, ebx
	setne	dl
	and	eax, -2
	add	esi, 2
	or	eax, edx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L1412:
	movzx	edx, WORD PTR [esi]
	movzx	ecx, dl
	movzx	eax, dh
	mov	ebx, DWORD PTR [edi+4+eax*8]
	mov	edx, DWORD PTR [edi+eax*8]
	sub	DWORD PTR [edi+ecx*8], edx
	sbb	DWORD PTR [edi+4+ecx*8], ebx
	add	esi, 2
	jmp	.L845
.L1413:
	movzx	ebx, WORD PTR [esi]
	add	esi, 2
	movzx	edx, bl
	movzx	eax, bh
	lea	edx, [edi+edx*8]
	mov	ecx, DWORD PTR [edi+eax*8]
	and	DWORD PTR [edx], ecx
	mov	ebx, DWORD PTR [edi+4+eax*8]
	and	DWORD PTR [edx+4], ebx
	jmp	.L845
.L811:
	test	al, al
	jns	.L812
	fld	QWORD PTR [ebp-48]
	fld	QWORD PTR [esi]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1298
.L1362:
	not	BYTE PTR [esi]
	jmp	.L866
.L1396:
	mov	BYTE PTR [esi], cl
	mov	esi, edx
	jmp	.L845
.L1379:
	fld	DWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], ebx
	fld	DWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1309
.L778:
	test	al, al
	jns	.L1295
	lea	eax, [edi+edx*8]
	fld	QWORD PTR [ebp-48]
	mov	esi, ecx
	fmul	QWORD PTR [eax]
	fstp	QWORD PTR [eax]
	jmp	.L845
.L1378:
	fld	DWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], ebx
	fld	DWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1305
.L804:
	test	al, al
	jns	.L812
	mov	DWORD PTR [ebp-48], ebx
	mov	DWORD PTR [ebp-44], esi
	fld	QWORD PTR [ebp-48]
	fld	QWORD PTR [edi+edx*8]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1298
.L1383:
	fld	DWORD PTR [esi]
	fld	DWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1309
.L1382:
	fld1
	fadd	DWORD PTR [ebx]
	fstp	DWORD PTR [ebx]
	jmp	.L866
.L1381:
	fld1
	fsubr	DWORD PTR [ebx]
	fstp	DWORD PTR [ebx]
	jmp	.L866
.L1380:
	fld	DWORD PTR [esi]
	fchs
	fstp	DWORD PTR [esi]
	jmp	.L866
.L1384:
	fld	DWORD PTR [esi]
	fld	DWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1305
.L765:
	test	al, al
	jns	.L1294
	lea	eax, [edi+edx*8]
	fld	QWORD PTR [ebp-48]
	mov	esi, ecx
	fsubr	QWORD PTR [eax]
	fstp	QWORD PTR [eax]
	jmp	.L845
.L1385:
	fld	DWORD PTR [esi]
	fld	DWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1301
.L798:
	test	al, al
	jns	.L1296
	fld	QWORD PTR [ebp-48]
	mov	esi, ecx
	fdivr	QWORD PTR [edx]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L1377:
	fld	DWORD PTR [edi+edx*8]
	mov	DWORD PTR [ebp-48], ebx
	fld	DWORD PTR [ebp-48]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1301
.L1376:
	lea	edx, [edi+edx*8]
	fld	DWORD PTR [ebp-48]
	mov	esi, ecx
	fdivr	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
	jmp	.L845
.L1386:
	fld	DWORD PTR [ebp-48]
	mov	esi, ecx
	fsubr	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
	jmp	.L845
.L1391:
	movzx	eax, BYTE PTR [esi]
	div	cl
	movzx	ecx, ah
	mov	BYTE PTR [esi], cl
	jmp	.L451
.L1407:
	test	al, 64
	lea	ecx, [esi+2]
	movzx	ebx, WORD PTR [esi]
	je	.L769
	movzx	esi, bl
	movzx	ebx, bh
	lea	eax, [edi+esi*8]
	mov	esi, ecx
	fld	DWORD PTR [eax]
	fsub	DWORD PTR [edi+ebx*8]
	fstp	DWORD PTR [eax]
	jmp	.L845
.L1398:
	or	BYTE PTR [esi], cl
	mov	esi, edx
	jmp	.L845
.L1397:
	movzx	eax, BYTE PTR [esi]
	div	cl
	mov	BYTE PTR [esi], al
	jmp	.L435
.L1364:
	add	BYTE PTR [esi], 1
	jmp	.L866
.L1399:
	and	BYTE PTR [esi], cl
	mov	esi, edx
	jmp	.L845
.L520:
	test	al, 32
	je	.L522
	movzx	eax, WORD PTR [ebx]
	sal	eax, cl
	mov	WORD PTR [ebx], ax
	jmp	.L521
.L418:
	test	al, 32
	je	.L420
	movzx	eax, WORD PTR [ebp-52]
	imul	ax, WORD PTR [ecx]
	mov	WORD PTR [ecx], ax
	jmp	.L419
.L1368:
	neg	BYTE PTR [esi]
	jmp	.L866
.L1392:
	cmp	BYTE PTR [esi], dl
	jmp	.L1287
.L654:
	test	al, 32
	je	.L656
	cmp	WORD PTR [esi], cx
	jmp	.L1281
.L1390:
	movzx	ebx, BYTE PTR [ebp-48]
	mov	esi, ecx
	add	BYTE PTR [edx], bl
	jmp	.L845
.L1402:
	movzx	edx, WORD PTR [esi]
	movzx	ecx, dl
	movzx	eax, dh
	mov	ebx, DWORD PTR [edi+4+eax*8]
	mov	edx, DWORD PTR [edi+eax*8]
	add	DWORD PTR [edi+ecx*8], edx
	adc	DWORD PTR [edi+4+ecx*8], ebx
	add	esi, 2
	jmp	.L845
.L1401:
	movzx	ebx, WORD PTR [esi]
	add	esi, 2
	movzx	edx, bl
	movzx	eax, bh
	lea	edx, [edi+edx*8]
	mov	ecx, DWORD PTR [edi+eax*8]
	xor	DWORD PTR [edx], ecx
	mov	ebx, DWORD PTR [edi+4+eax*8]
	xor	DWORD PTR [edx+4], ebx
	jmp	.L845
.L619:
	test	al, 4
	je	.L845
	movzx	ebx, WORD PTR [esi]
	mov	edx, DWORD PTR [esi+2]
	lea	ecx, [esi+6]
	movzx	esi, bh
	add	edx, DWORD PTR [edi+esi*8]
	test	al, 16
	je	.L622
	movzx	eax, bl
	movzx	edx, BYTE PTR [edx]
	cmp	BYTE PTR [edi+eax*8], dl
	jmp	.L1280
.L1389:
	cmp	BYTE PTR [esi], cl
	jmp	.L1279
.L1395:
	xor	BYTE PTR [esi], cl
	mov	esi, edx
	jmp	.L845
.L759:
	test	al, al
	jns	.L1293
	fld	QWORD PTR [ebp-48]
	mov	esi, ecx
	fadd	QWORD PTR [edx]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L1366:
	sub	BYTE PTR [esi], 1
	jmp	.L866
.L536:
	test	al, 32
	je	.L538
	movzx	eax, WORD PTR [ebx]
	sar	eax, cl
	mov	WORD PTR [ebx], ax
	jmp	.L537
.L1393:
	movzx	ebx, BYTE PTR [ebp-48]
	mov	esi, ecx
	sub	BYTE PTR [edx], bl
	jmp	.L845
.L752:
	test	al, al
	jns	.L1293
	lea	eax, [edi+edx*8]
	fld	QWORD PTR [ebp-48]
	mov	esi, ecx
	fadd	QWORD PTR [eax]
	fstp	QWORD PTR [eax]
	jmp	.L845
.L1388:
	cmp	BYTE PTR [esi], dl
	jmp	.L1283
.L1404:
	movzx	eax, WORD PTR [esi]
	movzx	ebx, al
	movzx	edx, ah
	lea	ebx, [edi+ebx*8]
	mov	ecx, DWORD PTR [edi+edx*8]
	mov	eax, DWORD PTR [ebx]
	mov	edx, DWORD PTR [ebx+4]
	shrd	eax, edx, cl
	shr	edx, cl
	test	cl, 32
	je	.L1424
	mov	eax, edx
	xor	edx, edx
.L1424:
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	add	esi, 2
	jmp	.L845
.L1400:
	movzx	ecx, WORD PTR [esi]
	lea	ebx, [esi+2]
	movzx	esi, cl
	movzx	edx, ch
	lea	esi, [edi+esi*8]
	push	DWORD PTR [edi+4+edx*8]
	push	DWORD PTR [edi+edx*8]
	push	DWORD PTR [esi+4]
	push	DWORD PTR [esi]
	call	__umoddi3
	add	esp, 16
	mov	DWORD PTR [esi], eax
	mov	DWORD PTR [esi+4], edx
	mov	esi, ebx
	jmp	.L845
.L1403:
	movzx	eax, WORD PTR [esi]
	add	esi, 2
	movzx	edx, al
	movzx	eax, ah
	lea	ebx, [edi+edx*8]
	mov	edx, DWORD PTR [edi+4+eax*8]
	mov	ecx, DWORD PTR [ebx+4]
	imul	edx, DWORD PTR [ebx]
	imul	ecx, DWORD PTR [edi+eax*8]
	mov	eax, DWORD PTR [edi+eax*8]
	add	ecx, edx
	mul	DWORD PTR [ebx]
	add	edx, ecx
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L845
.L576:
	test	al, 32
	je	.L578
	cmp	WORD PTR [esi], cx
	jmp	.L1275
.L1394:
	cmp	BYTE PTR [esi], cl
	jmp	.L1277
.L1405:
	movzx	ebx, WORD PTR [esi]
	add	esi, 2
	movzx	edx, bl
	movzx	eax, bh
	lea	edx, [edi+edx*8]
	mov	ecx, DWORD PTR [edi+eax*8]
	or	DWORD PTR [edx], ecx
	mov	ebx, DWORD PTR [edi+4+eax*8]
	or	DWORD PTR [edx+4], ebx
	jmp	.L845
.L1406:
	test	al, 64
	lea	ecx, [esi+2]
	movzx	ebx, WORD PTR [esi]
	je	.L782
	movzx	esi, bl
	movzx	ebx, bh
	lea	eax, [edi+esi*8]
	mov	esi, ecx
	fld	DWORD PTR [eax]
	fmul	DWORD PTR [edi+ebx*8]
	fstp	DWORD PTR [eax]
	jmp	.L845
.L1408:
	test	al, 64
	lea	ecx, [esi+2]
	movzx	ebx, WORD PTR [esi]
	je	.L808
	movzx	edx, bh
	movzx	esi, bl
	fld	DWORD PTR [edi+edx*8]
	fld	DWORD PTR [edi+esi*8]
	fxch	st(1)
	fcomip	st, st(1)
	fstp	st(0)
	jmp	.L1298
.L1409:
	test	al, 64
	lea	ecx, [esi+2]
	movzx	ebx, WORD PTR [esi]
	je	.L795
	movzx	esi, bl
	movzx	ebx, bh
	lea	eax, [edi+esi*8]
	mov	esi, ecx
	fld	DWORD PTR [eax]
	fdiv	DWORD PTR [edi+ebx*8]
	fstp	DWORD PTR [eax]
	jmp	.L845
.L1387:
	fld	DWORD PTR [ebp-48]
	mov	esi, ecx
	fmul	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
	jmp	.L845
.L643:
	xor	edx, edx
	jmp	.L642
.L565:
	xor	edx, edx
	jmp	.L564
.L591:
	xor	edx, edx
	jmp	.L590
.L617:
	xor	edx, edx
	jmp	.L616
.L621:
	xor	ecx, ecx
	jmp	.L620
.L647:
	xor	ecx, ecx
	jmp	.L646
.L595:
	xor	ecx, ecx
	jmp	.L594
.L569:
	xor	ecx, ecx
	jmp	.L568
.L1258:
	mov	DWORD PTR [ebp-68], esi
	mov	ebx, 1
	mov	DWORD PTR [ebp-52], edi
	mov	esi, DWORD PTR [ebp-48]
	jmp	.L734
.L1375:
	fadd	DWORD PTR .LC4
	jmp	.L748
.L1374:
	fadd	DWORD PTR .LC4
	jmp	.L745
.L873:
	xor	eax, eax
	jmp	.L355
.L553:
	test	al, 64
	je	.L554
	sub	DWORD PTR [esi], 1
	jmp	.L866
.L692:
	test	al, 64
	je	.L693
	cmp	DWORD PTR [esi], edx
	jmp	.L1287
.L769:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	ecx, bl
	movzx	edx, bh
	lea	eax, [edi+ecx*8]
	fld	QWORD PTR [eax]
	fsub	QWORD PTR [edi+edx*8]
	fstp	QWORD PTR [eax]
	jmp	.L845
.L538:
	test	al, 64
	je	.L539
	shr	DWORD PTR [ebx], cl
	jmp	.L537
.L821:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	ecx, bl
	movzx	edx, bh
	fld	QWORD PTR [edi+ecx*8]
.L1303:
	fld	QWORD PTR [edi+edx*8]
	fxch	st(1)
.L1302:
	movzx	eax, BYTE PTR [edi+212]
	fcomip	st, st(1)
	fstp	st(0)
	seta	dl
	and	eax, -2
	or	eax, edx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L582:
	test	al, 32
	je	.L583
	movzx	esi, bh
	movzx	ebx, WORD PTR [edi+esi*8]
	cmp	WORD PTR [edx], bx
	jmp	.L1276
.L484:
	test	al, 64
	je	.L485
	or	DWORD PTR [esi], ecx
	mov	esi, edx
	jmp	.L845
.L438:
	test	al, 32
	je	.L439
	movzx	eax, WORD PTR [ebx]
	movzx	esi, dh
	xor	edx, edx
	div	WORD PTR [edi+esi*8]
	mov	esi, ecx
	mov	WORD PTR [ebx], ax
	jmp	.L845
.L436:
	test	al, 64
	je	.L437
	mov	eax, DWORD PTR [esi]
	xor	edx, edx
	div	ecx
	mov	DWORD PTR [esi], eax
	jmp	.L435
.L468:
	test	al, 64
	je	.L469
	and	DWORD PTR [esi], ecx
	mov	esi, edx
	jmp	.L845
.L676:
	test	al, 64
	je	.L677
	cmp	DWORD PTR [esi], edx
	jmp	.L1283
.L547:
	test	al, 64
	je	.L548
	add	DWORD PTR [esi], 1
	jmp	.L866
.L372:
	test	al, 64
	je	.L373
	mov	DWORD PTR [esi], ecx
	mov	esi, edx
	jmp	.L845
.L522:
	test	al, 64
	je	.L523
	sal	DWORD PTR [ebx], cl
	jmp	.L521
.L578:
	test	al, 64
	je	.L579
	cmp	DWORD PTR [esi], ecx
	jmp	.L1275
.L701:
	test	al, 32
	je	.L702
	movsx	esi, WORD PTR [edx]
	add	esi, ecx
	jmp	.L845
.L604:
	test	al, 64
	je	.L605
	cmp	DWORD PTR [esi], ecx
	jmp	.L1277
.L406:
	test	al, 32
	je	.L407
	movzx	eax, dh
	movzx	esi, WORD PTR [edi+eax*8]
	sub	WORD PTR [ecx], si
	mov	esi, ebx
	jmp	.L845
.L509:
	test	al, 64
	je	.L510
	not	DWORD PTR [esi]
	jmp	.L866
.L524:
	test	al, 32
	je	.L525
	movzx	esi, ch
	movzx	eax, WORD PTR [ebx]
	movzx	ecx, WORD PTR [edi+esi*8]
	mov	esi, edx
	sal	eax, cl
	mov	WORD PTR [ebx], ax
	jmp	.L845
.L374:
	test	al, 32
	je	.L375
	movzx	eax, dh
	movzx	esi, WORD PTR [edi+eax*8]
	mov	WORD PTR [ecx], si
	mov	esi, ebx
	jmp	.L845
.L422:
	test	al, 32
	je	.L423
	movzx	esi, bh
	movzx	ebx, WORD PTR [ecx]
	imul	bx, WORD PTR [edi+esi*8]
	mov	esi, edx
	mov	WORD PTR [ecx], bx
	jmp	.L845
.L787:
	test	al, al
	mov	esi, DWORD PTR [ebp-48]
	jns	.L845
	fld	QWORD PTR [edx]
	movzx	eax, ch
	fmul	QWORD PTR [edi+eax*8]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L715:
	test	al, 32
	je	.L716
	test	BYTE PTR [edi+212], 1
	je	.L845
	movsx	eax, WORD PTR [edx]
	add	esi, eax
	jmp	.L845
.L470:
	test	al, 32
	je	.L471
	movzx	eax, dh
	movzx	esi, WORD PTR [edi+eax*8]
	and	WORD PTR [ecx], si
	mov	esi, ebx
	jmp	.L845
.L630:
	test	al, 64
	je	.L631
	cmp	DWORD PTR [esi], ecx
	jmp	.L1279
.L847:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, bl
	movzx	ecx, bh
	fld	QWORD PTR [edi+eax*8]
	fld	QWORD PTR [edi+ecx*8]
.L1311:
	movzx	ebx, BYTE PTR [edi+212]
	fcomip	st, st(1)
	fstp	st(0)
	setne	dl
	and	ebx, -2
	or	ebx, edx
	mov	BYTE PTR [edi+212], bl
	jmp	.L845
.L390:
	test	al, 32
	je	.L391
	movzx	eax, dh
	movzx	esi, WORD PTR [edi+eax*8]
	add	WORD PTR [ecx], si
	mov	esi, ebx
	jmp	.L845
.L813:
	test	al, al
	mov	esi, DWORD PTR [ebp-48]
	jns	.L845
	movzx	eax, dh
	fld	QWORD PTR [edi+eax*8]
	fld	QWORD PTR [ecx]
	fxch	st(1)
.L1299:
	movzx	eax, BYTE PTR [edi+212]
	fcomip	st, st(1)
	fstp	st(0)
	seta	bl
	and	eax, -2
	or	eax, ebx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L388:
	test	al, 64
	je	.L389
	mov	ebx, DWORD PTR [ebp-48]
	mov	esi, ecx
	add	DWORD PTR [edx], ebx
	jmp	.L845
.L795:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	ecx, bl
	movzx	edx, bh
	lea	eax, [edi+ecx*8]
	fld	QWORD PTR [eax]
	fdiv	QWORD PTR [edi+edx*8]
	fstp	QWORD PTR [eax]
	jmp	.L845
.L782:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	ecx, bl
	movzx	edx, bh
	lea	eax, [edi+ecx*8]
	fld	QWORD PTR [eax]
	fmul	QWORD PTR [edi+edx*8]
	fstp	QWORD PTR [eax]
	jmp	.L845
.L808:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	edx, bh
	movzx	ecx, bl
	fld	QWORD PTR [edi+edx*8]
	fld	QWORD PTR [edi+ecx*8]
	fxch	st(1)
	jmp	.L1299
.L852:
	test	al, al
	mov	esi, DWORD PTR [ebp-48]
	jns	.L845
	movzx	edx, dh
.L1310:
	fld	QWORD PTR [edi+edx*8]
	fld	QWORD PTR [ecx]
	jmp	.L1311
.L694:
	test	al, 32
	je	.L695
	movzx	edx, dh
	movzx	eax, WORD PTR [ecx]
	cmp	WORD PTR [edi+edx*8], ax
	jmp	.L1289
.L486:
	test	al, 32
	je	.L487
	movzx	eax, dh
	movzx	esi, WORD PTR [edi+eax*8]
	or	WORD PTR [ecx], si
	mov	esi, ebx
	jmp	.L845
.L500:
	test	al, 64
	je	.L501
	xor	DWORD PTR [esi], ecx
	mov	esi, edx
	jmp	.L845
.L452:
	test	al, 64
	je	.L453
	mov	eax, DWORD PTR [esi]
	xor	edx, edx
	div	ecx
	mov	DWORD PTR [esi], edx
	jmp	.L451
.L839:
	test	al, al
	mov	esi, DWORD PTR [ebp-48]
	jns	.L845
	movzx	edx, dh
.L1306:
	fld	QWORD PTR [edi+edx*8]
	fld	QWORD PTR [ecx]
.L1307:
	movzx	ebx, BYTE PTR [edi+212]
	fcomip	st, st(1)
	fstp	st(0)
	sete	dl
	and	ebx, -2
	or	ebx, edx
	mov	BYTE PTR [edi+212], bl
	jmp	.L845
.L420:
	test	al, 64
	je	.L421
	mov	esi, DWORD PTR [ebp-52]
	imul	esi, DWORD PTR [ecx]
	mov	DWORD PTR [ecx], esi
	jmp	.L419
.L708:
	test	al, 32
	je	.L709
	test	BYTE PTR [edi+212], 1
	jne	.L845
	movsx	eax, WORD PTR [edx]
	add	esi, eax
	jmp	.L845
.L502:
	test	al, 32
	je	.L503
	movzx	eax, dh
	movzx	esi, WORD PTR [edi+eax*8]
	xor	WORD PTR [ecx], si
	mov	esi, ebx
	jmp	.L845
.L404:
	test	al, 64
	je	.L405
	mov	ebx, DWORD PTR [ebp-48]
	mov	esi, ecx
	sub	DWORD PTR [edx], ebx
	jmp	.L845
.L826:
	test	al, al
	mov	esi, DWORD PTR [ebp-48]
	jns	.L845
	movzx	edx, dh
	fld	QWORD PTR [ecx]
	jmp	.L1303
.L834:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, bl
	movzx	ecx, bh
	fld	QWORD PTR [edi+eax*8]
	fld	QWORD PTR [edi+ecx*8]
	jmp	.L1307
.L756:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	ecx, bl
	movzx	edx, bh
	lea	eax, [edi+ecx*8]
	fld	QWORD PTR [eax]
	fadd	QWORD PTR [edi+edx*8]
	fstp	QWORD PTR [eax]
	jmp	.L845
.L761:
	test	al, al
	mov	esi, DWORD PTR [ebp-48]
	jns	.L845
	fld	QWORD PTR [edx]
	movzx	eax, ch
	fadd	QWORD PTR [edi+eax*8]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L634:
	test	al, 32
	je	.L635
	movzx	esi, bh
	movzx	ebx, WORD PTR [edi+esi*8]
	cmp	WORD PTR [edx], bx
	jmp	.L1280
.L678:
	test	al, 32
	je	.L679
	movzx	edx, dh
	movzx	eax, WORD PTR [ecx]
	cmp	WORD PTR [edi+edx*8], ax
	jmp	.L1285
.L774:
	test	al, al
	mov	esi, DWORD PTR [ebp-48]
	jns	.L845
	fld	QWORD PTR [edx]
	movzx	eax, ch
	fsub	QWORD PTR [edi+eax*8]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L800:
	test	al, al
	mov	esi, DWORD PTR [ebp-48]
	jns	.L845
	fld	QWORD PTR [edx]
	movzx	eax, ch
	fdiv	QWORD PTR [edi+eax*8]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L656:
	test	al, 64
	je	.L657
	cmp	DWORD PTR [esi], ecx
	jmp	.L1281
.L559:
	test	al, 64
	je	.L560
	neg	DWORD PTR [esi]
	jmp	.L866
.L660:
	test	al, 32
	je	.L661
	movzx	esi, bh
	movzx	ebx, WORD PTR [edi+esi*8]
	cmp	WORD PTR [edx], bx
	jmp	.L1282
.L608:
	test	al, 32
	je	.L609
	movzx	esi, bh
	movzx	ebx, WORD PTR [edi+esi*8]
	cmp	WORD PTR [edx], bx
	jmp	.L1278
.L454:
	test	al, 32
	je	.L455
	movzx	eax, WORD PTR [ebx]
	movzx	esi, dh
	xor	edx, edx
	div	WORD PTR [edi+esi*8]
	mov	esi, ecx
	mov	WORD PTR [ebx], dx
	jmp	.L845
.L540:
	test	al, 32
	je	.L541
	movzx	eax, WORD PTR [esi]
	movzx	esi, ch
	movzx	ecx, WORD PTR [edi+esi*8]
	mov	esi, edx
	sar	eax, cl
	mov	WORD PTR [ebx], ax
	jmp	.L845
.L835:
	test	al, al
	mov	esi, ebx
	movzx	edx, dl
	jns	.L845
	jmp	.L1306
.L570:
	test	al, 32
	je	.L571
	movzx	esi, bl
	movzx	ebx, WORD PTR [edx]
	cmp	WORD PTR [edi+esi*8], bx
	jmp	.L1276
.L579:
	test	al, al
	jns	.L577
	mov	ebx, DWORD PTR [ebp-48]
	cmp	DWORD PTR [esi+4], ebx
	mov	eax, 1
	jl	.L580
	jg	.L581
	cmp	DWORD PTR [esi], ecx
	jnb	.L581
.L580:
	movzx	ecx, BYTE PTR [edi+212]
	mov	esi, edx
	and	ecx, -2
	or	eax, ecx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L581:
	xor	eax, eax
	jmp	.L580
.L407:
	test	al, 64
	je	.L408
	movzx	esi, dh
	mov	edx, DWORD PTR [edi+esi*8]
	mov	esi, ebx
	sub	DWORD PTR [ecx], edx
	jmp	.L845
.L541:
	test	al, 64
	je	.L542
	movzx	esi, ch
	mov	ecx, DWORD PTR [edi+esi*8]
	mov	esi, edx
	shr	DWORD PTR [ebx], cl
	jmp	.L845
.L539:
	test	al, al
	jns	.L537
	mov	edx, DWORD PTR [ebx+4]
	mov	eax, DWORD PTR [ebx]
	xor	esi, esi
	shrd	eax, edx, cl
	shr	edx, cl
	test	cl, 32
	cmovne	eax, edx
	cmovne	edx, esi
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L537
.L389:
	test	al, al
	jns	.L387
	mov	eax, DWORD PTR [ebp-48]
	add	DWORD PTR [edx], eax
	mov	esi, DWORD PTR [ebp-44]
	adc	DWORD PTR [edx+4], esi
	mov	esi, ecx
	jmp	.L845
.L510:
	test	al, al
	jns	.L866
	not	DWORD PTR [esi]
	not	DWORD PTR [esi+4]
	jmp	.L866
.L796:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, dl
	lea	edx, [edi+eax*8]
	fld	QWORD PTR [edx]
	fdiv	QWORD PTR [ecx]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L809:
	test	al, al
	mov	esi, ebx
	jns	.L845
	fld	QWORD PTR [ecx]
	movzx	eax, dl
	fld	QWORD PTR [edi+eax*8]
	fxch	st(1)
	jmp	.L1299
.L414:
	test	al, 32
	je	.L415
	movzx	ebx, bl
	lea	esi, [edi+ebx*8]
	movzx	eax, WORD PTR [esi]
	imul	ax, WORD PTR [edx]
	mov	WORD PTR [esi], ax
	mov	esi, ecx
	jmp	.L845
.L382:
	test	al, 32
	je	.L383
	movzx	edx, dl
	movzx	eax, WORD PTR [ebx]
	mov	esi, ecx
	add	WORD PTR [edi+edx*8], ax
	jmp	.L845
.L716:
	test	al, 64
	jne	.L1356
	test	al, al
	jns	.L845
.L1356:
	test	BYTE PTR [edi+212], 1
	je	.L845
	add	esi, DWORD PTR [edx]
	jmp	.L845
.L695:
	test	al, 64
	je	.L696
	movzx	esi, dh
.L1290:
	mov	eax, DWORD PTR [ecx]
	cmp	DWORD PTR [edi+esi*8], eax
	jmp	.L1289
.L391:
	test	al, 64
	je	.L392
	movzx	esi, dh
	mov	edx, DWORD PTR [edi+esi*8]
	mov	esi, ebx
	add	DWORD PTR [ecx], edx
	jmp	.L845
.L648:
	test	al, 32
	je	.L649
	movzx	esi, bl
	movzx	ebx, WORD PTR [edx]
	cmp	WORD PTR [edi+esi*8], bx
	jmp	.L1282
.L516:
	test	al, 32
	je	.L517
	movzx	eax, cl
	movzx	ecx, WORD PTR [edx]
	lea	esi, [edi+eax*8]
	movzx	eax, WORD PTR [esi]
	sal	eax, cl
	mov	WORD PTR [esi], ax
	mov	esi, ebx
	jmp	.L845
.L523:
	test	al, al
	jns	.L521
	mov	eax, DWORD PTR [ebx]
	mov	edx, DWORD PTR [ebx+4]
	xor	esi, esi
	shld	edx, eax, cl
	sal	eax, cl
	test	cl, 32
	cmovne	edx, eax
	cmovne	eax, esi
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L521
.L757:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, dl
	lea	edx, [edi+eax*8]
	fld	QWORD PTR [edx]
	fadd	QWORD PTR [ecx]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L423:
	test	al, 64
	je	.L424
	movzx	ebx, bh
	mov	eax, DWORD PTR [ecx]
	mov	esi, edx
	imul	eax, DWORD PTR [edi+ebx*8]
	mov	DWORD PTR [ecx], eax
	jmp	.L845
.L421:
	test	al, al
	jns	.L419
	mov	ebx, DWORD PTR [ebp-52]
	mov	esi, DWORD PTR [ecx+4]
	mov	eax, DWORD PTR [ecx]
	imul	esi, ebx
	imul	eax, edx
	add	esi, eax
	mov	eax, ebx
	mul	DWORD PTR [ecx]
	add	edx, esi
	mov	DWORD PTR [ecx], eax
	mov	DWORD PTR [ecx+4], edx
	jmp	.L419
.L635:
	test	al, 64
	je	.L636
	movzx	ebx, bh
	mov	eax, DWORD PTR [edi+ebx*8]
	cmp	DWORD PTR [edx], eax
	jmp	.L1280
.L596:
	test	al, 32
	je	.L597
	movzx	esi, bl
	movzx	ebx, WORD PTR [edx]
	cmp	WORD PTR [edi+esi*8], bx
	jmp	.L1278
.L702:
	test	al, 64
	je	.L703
	add	ecx, DWORD PTR [edx]
	mov	esi, ecx
	jmp	.L845
.L770:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, dl
	lea	edx, [edi+eax*8]
	fld	QWORD PTR [edx]
	fsub	QWORD PTR [ecx]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L366:
	test	al, 32
	je	.L367
	movzx	eax, WORD PTR [ecx]
	movzx	edx, dl
	mov	esi, ebx
	mov	WORD PTR [edi+edx*8], ax
	jmp	.L845
.L661:
	test	al, 64
	je	.L662
	movzx	ebx, bh
	mov	eax, DWORD PTR [edi+ebx*8]
	cmp	DWORD PTR [edx], eax
	jmp	.L1282
.L478:
	test	al, 32
	je	.L479
	movzx	edx, dl
	movzx	ecx, WORD PTR [ecx]
	mov	esi, ebx
	or	WORD PTR [edi+edx*8], cx
	jmp	.L845
.L503:
	test	al, 64
	je	.L504
	movzx	esi, dh
	mov	edx, DWORD PTR [edi+esi*8]
	mov	esi, ebx
	xor	DWORD PTR [ecx], edx
	jmp	.L845
.L501:
	test	al, al
	jns	.L499
	xor	DWORD PTR [esi], ecx
	mov	eax, DWORD PTR [ebp-48]
	xor	DWORD PTR [esi+4], eax
	mov	esi, edx
	jmp	.L845
.L471:
	test	al, 64
	je	.L472
	movzx	esi, dh
	mov	edx, DWORD PTR [edi+esi*8]
	mov	esi, ebx
	and	DWORD PTR [ecx], edx
	jmp	.L845
.L455:
	test	al, 64
	je	.L456
	mov	eax, DWORD PTR [ebx]
	movzx	esi, dh
	xor	edx, edx
	div	DWORD PTR [edi+esi*8]
	mov	esi, ecx
	mov	DWORD PTR [ebx], edx
	jmp	.L845
.L848:
	test	al, al
	mov	esi, ebx
	movzx	edx, dl
	jns	.L845
	jmp	.L1310
.L783:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, dl
	lea	edx, [edi+eax*8]
	fld	QWORD PTR [edx]
	fmul	QWORD PTR [ecx]
	fstp	QWORD PTR [edx]
	jmp	.L845
.L657:
	test	al, al
	jns	.L655
	mov	ebx, DWORD PTR [ebp-48]
	cmp	DWORD PTR [esi+4], ebx
	mov	eax, 1
	ja	.L658
	jb	.L659
	cmp	DWORD PTR [esi], ecx
	jbe	.L659
.L658:
	movzx	ecx, BYTE PTR [edi+212]
	mov	esi, edx
	and	ecx, -2
	or	eax, ecx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L659:
	xor	eax, eax
	jmp	.L658
.L525:
	test	al, 64
	je	.L526
	movzx	esi, ch
	mov	ecx, DWORD PTR [edi+esi*8]
	mov	esi, edx
	sal	DWORD PTR [ebx], cl
	jmp	.L845
.L1415:
	mov	eax, DWORD PTR [ebx+edx]
	lea	esi, [esi+5]
	mov	DWORD PTR [ebp-60], esi
	add	eax, -1
	jmp	.L720
.L583:
	test	al, 64
	je	.L584
	movzx	ebx, bh
	mov	eax, DWORD PTR [edi+ebx*8]
	cmp	DWORD PTR [edx], eax
	jmp	.L1276
.L469:
	test	al, al
	jns	.L467
	and	DWORD PTR [esi], ecx
	mov	eax, DWORD PTR [ebp-48]
	and	DWORD PTR [esi+4], eax
	mov	esi, edx
	jmp	.L845
.L439:
	test	al, 64
	je	.L440
	mov	eax, DWORD PTR [ebx]
	movzx	esi, dh
	xor	edx, edx
	div	DWORD PTR [edi+esi*8]
	mov	esi, ecx
	mov	DWORD PTR [ebx], eax
	jmp	.L845
.L548:
	test	al, al
	jns	.L866
	add	DWORD PTR [esi], 1
	adc	DWORD PTR [esi+4], 0
	jmp	.L866
.L485:
	test	al, al
	jns	.L483
	or	DWORD PTR [esi], ecx
	mov	eax, DWORD PTR [ebp-48]
	or	DWORD PTR [esi+4], eax
	mov	esi, edx
	jmp	.L845
.L677:
	test	al, al
	jns	.L675
	xor	edx, DWORD PTR [esi]
	mov	eax, DWORD PTR [ebp-48]
	xor	eax, DWORD PTR [esi+4]
	or	edx, eax
	jmp	.L1283
.L437:
	test	al, al
	jns	.L435
	push	ebx
	push	ecx
	push	DWORD PTR [esi+4]
	push	DWORD PTR [esi]
	call	__udivdi3
	add	esp, 16
	mov	DWORD PTR [esi], eax
	mov	DWORD PTR [esi+4], edx
	jmp	.L435
.L373:
	test	al, al
	jns	.L371
	mov	eax, DWORD PTR [ebp-48]
	mov	DWORD PTR [esi], ecx
	mov	DWORD PTR [esi+4], eax
	mov	esi, edx
	jmp	.L845
.L430:
	test	al, 32
	je	.L431
	movzx	ebx, bl
	mov	esi, edx
	xor	edx, edx
	lea	ebx, [edi+ebx*8]
	movzx	eax, WORD PTR [ebx]
	div	WORD PTR [esi]
	mov	esi, ecx
	mov	WORD PTR [ebx], ax
	jmp	.L845
.L670:
	test	al, 32
	je	.L671
	movzx	esi, dl
	movzx	ecx, WORD PTR [ecx]
	cmp	WORD PTR [edi+esi*8], cx
	jmp	.L1285
.L554:
	test	al, al
	jns	.L866
	add	DWORD PTR [esi], -1
	adc	DWORD PTR [esi+4], -1
	jmp	.L866
.L622:
	test	al, 32
	je	.L623
	movzx	esi, bl
	movzx	ebx, WORD PTR [edx]
	cmp	WORD PTR [edi+esi*8], bx
	jmp	.L1280
.L532:
	test	al, 32
	je	.L533
	movzx	eax, cl
	movzx	ecx, WORD PTR [edx]
	lea	esi, [edi+eax*8]
	movzx	eax, WORD PTR [esi]
	sar	eax, cl
	mov	WORD PTR [esi], ax
	mov	esi, ebx
	jmp	.L845
.L693:
	test	al, al
	jns	.L691
	xor	edx, DWORD PTR [esi]
	mov	eax, DWORD PTR [ebp-48]
	xor	eax, DWORD PTR [esi+4]
	or	edx, eax
	jmp	.L1287
.L446:
	test	al, 32
	je	.L447
	movzx	ebx, bl
	mov	esi, edx
	xor	edx, edx
	lea	ebx, [edi+ebx*8]
	movzx	eax, WORD PTR [ebx]
	div	WORD PTR [esi]
	mov	esi, ecx
	mov	WORD PTR [ebx], dx
	jmp	.L845
.L822:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, dl
	fld	QWORD PTR [edi+eax*8]
	fld	QWORD PTR [ecx]
	fxch	st(1)
	jmp	.L1302
.L686:
	test	al, 32
	je	.L687
	movzx	esi, dl
	movzx	ecx, WORD PTR [ecx]
	cmp	WORD PTR [edi+esi*8], cx
	jmp	.L1289
.L631:
	test	al, al
	jns	.L629
	mov	ebx, DWORD PTR [ebp-48]
	cmp	DWORD PTR [esi+4], ebx
	mov	eax, 1
	jb	.L632
	ja	.L633
	cmp	DWORD PTR [esi], ecx
	jnb	.L633
.L632:
	movzx	ecx, BYTE PTR [edi+212]
	mov	esi, edx
	and	ecx, -2
	or	eax, ecx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L633:
	xor	eax, eax
	jmp	.L632
.L494:
	test	al, 32
	je	.L495
	movzx	edx, dl
	movzx	ecx, WORD PTR [ecx]
	mov	esi, ebx
	xor	WORD PTR [edi+edx*8], cx
	jmp	.L845
.L679:
	test	al, 64
	je	.L680
	movzx	esi, dh
.L1286:
	mov	eax, DWORD PTR [ecx]
	cmp	DWORD PTR [edi+esi*8], eax
	jmp	.L1285
.L487:
	test	al, 64
	je	.L488
	movzx	esi, dh
	mov	edx, DWORD PTR [edi+esi*8]
	mov	esi, ebx
	or	DWORD PTR [ecx], edx
	jmp	.L845
.L398:
	test	al, 32
	je	.L399
	movzx	edx, dl
	movzx	eax, WORD PTR [ebx]
	mov	esi, ecx
	sub	WORD PTR [edi+edx*8], ax
	jmp	.L845
.L709:
	test	al, 64
	jne	.L1354
	test	al, al
	jns	.L845
.L1354:
	test	BYTE PTR [edi+212], 1
	jne	.L845
	add	esi, DWORD PTR [edx]
	jmp	.L845
.L462:
	test	al, 32
	je	.L463
	movzx	edx, dl
	movzx	ecx, WORD PTR [ecx]
	mov	esi, ebx
	and	WORD PTR [edi+edx*8], cx
	jmp	.L845
.L405:
	test	al, al
	jns	.L403
	mov	eax, DWORD PTR [ebp-48]
	sub	DWORD PTR [edx], eax
	mov	esi, DWORD PTR [ebp-44]
	sbb	DWORD PTR [edx+4], esi
	mov	esi, ecx
	jmp	.L845
.L609:
	test	al, 64
	je	.L610
	movzx	ebx, bh
	mov	eax, DWORD PTR [edi+ebx*8]
	cmp	DWORD PTR [edx], eax
	jmp	.L1278
.L375:
	test	al, 64
	je	.L376
	movzx	esi, dh
	mov	edx, DWORD PTR [edi+esi*8]
	mov	esi, ebx
	mov	DWORD PTR [ecx], edx
	jmp	.L845
.L560:
	test	al, al
	jns	.L866
	neg	DWORD PTR [esi]
	adc	DWORD PTR [esi+4], 0
	neg	DWORD PTR [esi+4]
	jmp	.L866
.L605:
	test	al, al
	jns	.L603
	mov	ebx, DWORD PTR [ebp-48]
	cmp	DWORD PTR [esi+4], ebx
	mov	eax, 1
	jg	.L606
	jl	.L607
	cmp	DWORD PTR [esi], ecx
	jbe	.L607
.L606:
	movzx	ecx, BYTE PTR [edi+212]
	mov	esi, edx
	and	ecx, -2
	or	eax, ecx
	mov	BYTE PTR [edi+212], al
	jmp	.L845
.L607:
	xor	eax, eax
	jmp	.L606
.L453:
	test	al, al
	jns	.L451
	push	ebx
	push	ecx
	push	DWORD PTR [esi+4]
	push	DWORD PTR [esi]
	call	__umoddi3
	add	esp, 16
	mov	DWORD PTR [esi], eax
	mov	DWORD PTR [esi+4], edx
	jmp	.L451
.L870:
	mov	eax, -1
	jmp	.L337
.L447:
	test	al, 64
	je	.L448
	movzx	esi, bl
	lea	ebx, [edi+esi*8]
	mov	esi, edx
	xor	edx, edx
	mov	eax, DWORD PTR [ebx]
	div	DWORD PTR [esi]
	mov	esi, ecx
	mov	DWORD PTR [ebx], edx
	jmp	.L845
.L610:
	test	al, al
	mov	esi, ecx
	jns	.L845
	mov	esi, DWORD PTR [edx]
	movzx	eax, bh
	mov	ebx, 1
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edi+4+eax*8]
	cmp	DWORD PTR [edx+4], esi
	jg	.L611
	jl	.L612
	mov	edx, DWORD PTR [ebp-48]
	cmp	edx, DWORD PTR [edi+eax*8]
	jbe	.L612
.L611:
	movzx	edx, BYTE PTR [edi+212]
	mov	esi, ecx
	and	edx, -2
	or	ebx, edx
	mov	BYTE PTR [edi+212], bl
	jmp	.L845
.L598:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, bl
	mov	ebx, 1
	mov	esi, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edx+4]
	cmp	DWORD PTR [edi+4+eax*8], esi
	jg	.L611
	jl	.L612
	mov	eax, DWORD PTR [ebp-48]
	cmp	eax, DWORD PTR [edx]
	ja	.L611
.L612:
	xor	ebx, ebx
	jmp	.L611
.L399:
	test	al, 64
	je	.L400
	movzx	esi, dl
	mov	ebx, DWORD PTR [ebx]
	sub	DWORD PTR [edi+esi*8], ebx
	mov	esi, ecx
	jmp	.L845
.L376:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	edx, dh
	mov	eax, DWORD PTR [edi+edx*8]
	mov	ebx, DWORD PTR [edi+4+edx*8]
	mov	DWORD PTR [ecx], eax
	mov	DWORD PTR [ecx+4], ebx
	jmp	.L845
.L488:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	edx, dh
	mov	eax, DWORD PTR [edi+edx*8]
	mov	ebx, DWORD PTR [edi+4+edx*8]
	or	DWORD PTR [ecx], eax
	or	DWORD PTR [ecx+4], ebx
	jmp	.L845
.L597:
	test	al, 64
	je	.L598
	movzx	esi, bl
	mov	eax, DWORD PTR [edx]
	cmp	DWORD PTR [edi+esi*8], eax
	jmp	.L1278
.L479:
	test	al, 64
	je	.L480
	movzx	esi, dl
	mov	eax, DWORD PTR [ecx]
	or	DWORD PTR [edi+esi*8], eax
	mov	esi, ebx
	jmp	.L845
.L504:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	edx, dh
	mov	eax, DWORD PTR [edi+edx*8]
	mov	ebx, DWORD PTR [edi+4+edx*8]
	xor	DWORD PTR [ecx], eax
	xor	DWORD PTR [ecx+4], ebx
	jmp	.L845
.L431:
	test	al, 64
	je	.L432
	movzx	esi, bl
	lea	ebx, [edi+esi*8]
	mov	esi, edx
	xor	edx, edx
	mov	eax, DWORD PTR [ebx]
	div	DWORD PTR [esi]
	mov	esi, ecx
	mov	DWORD PTR [ebx], eax
	jmp	.L845
.L623:
	test	al, 64
	je	.L624
	movzx	esi, bl
	mov	eax, DWORD PTR [edx]
	cmp	DWORD PTR [edi+esi*8], eax
	jmp	.L1280
.L680:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	edx, dh
.L1284:
	mov	ebx, DWORD PTR [edi+edx*8]
	mov	eax, DWORD PTR [edi+4+edx*8]
	xor	ebx, DWORD PTR [ecx]
	xor	eax, DWORD PTR [ecx+4]
	movzx	edx, BYTE PTR [edi+212]
	or	ebx, eax
	sete	cl
	and	edx, -2
	or	edx, ecx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
.L526:
	test	al, al
	mov	esi, edx
	jns	.L845
	movzx	eax, ch
	mov	ecx, DWORD PTR [edi+eax*8]
.L1273:
	mov	eax, DWORD PTR [ebx]
	mov	edx, DWORD PTR [ebx+4]
	shld	edx, eax, cl
	sal	eax, cl
	test	cl, 32
	je	.L1423
	mov	edx, eax
	xor	eax, eax
.L1423:
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L845
.L542:
	test	al, al
	mov	esi, edx
	jns	.L845
	movzx	eax, ch
	mov	ecx, DWORD PTR [edi+eax*8]
.L1274:
	mov	edx, DWORD PTR [ebx+4]
	mov	eax, DWORD PTR [ebx]
	shrd	eax, edx, cl
	shr	edx, cl
	test	cl, 32
	je	.L1422
	mov	eax, edx
	xor	edx, edx
.L1422:
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L845
.L687:
	test	al, 64
	je	.L688
	movzx	esi, dl
	jmp	.L1290
.L495:
	test	al, 64
	je	.L496
	movzx	esi, dl
	mov	eax, DWORD PTR [ecx]
	xor	DWORD PTR [edi+esi*8], eax
	mov	esi, ebx
	jmp	.L845
.L463:
	test	al, 64
	je	.L464
	movzx	esi, dl
	mov	eax, DWORD PTR [ecx]
	and	DWORD PTR [edi+esi*8], eax
	mov	esi, ebx
	jmp	.L845
.L584:
	test	al, al
	mov	esi, ecx
	jns	.L845
	mov	esi, DWORD PTR [edx]
	movzx	eax, bh
	mov	ebx, 1
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edi+4+eax*8]
	cmp	DWORD PTR [edx+4], esi
	jl	.L585
	jg	.L586
	mov	edx, DWORD PTR [ebp-48]
	cmp	edx, DWORD PTR [edi+eax*8]
	jnb	.L586
.L585:
	movzx	edx, BYTE PTR [edi+212]
	mov	esi, ecx
	and	edx, -2
	or	ebx, edx
	mov	BYTE PTR [edi+212], bl
	jmp	.L845
.L572:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, bl
	mov	ebx, 1
	mov	esi, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edx+4]
	cmp	DWORD PTR [edi+4+eax*8], esi
	jl	.L585
	jg	.L586
	mov	eax, DWORD PTR [ebp-48]
	cmp	eax, DWORD PTR [edx]
	jb	.L585
.L586:
	xor	ebx, ebx
	jmp	.L585
.L671:
	test	al, 64
	je	.L672
	movzx	esi, dl
	jmp	.L1286
.L533:
	test	al, 64
	je	.L534
	movzx	esi, cl
	mov	ecx, DWORD PTR [edx]
	shr	DWORD PTR [edi+esi*8], cl
	mov	esi, ebx
	jmp	.L845
.L440:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, dh
	push	DWORD PTR [edi+4+eax*8]
	push	DWORD PTR [edi+eax*8]
	push	DWORD PTR [ebx+4]
	push	DWORD PTR [ebx]
	call	__udivdi3
	add	esp, 16
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L845
.L696:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	edx, dh
.L1288:
	mov	ebx, DWORD PTR [edi+edx*8]
	mov	eax, DWORD PTR [edi+4+edx*8]
	xor	ebx, DWORD PTR [ecx]
	xor	eax, DWORD PTR [ecx+4]
	movzx	edx, BYTE PTR [edi+212]
	or	ebx, eax
	setne	cl
	and	edx, -2
	or	edx, ecx
	mov	BYTE PTR [edi+212], dl
	jmp	.L845
.L649:
	test	al, 64
	je	.L650
	movzx	esi, bl
	mov	eax, DWORD PTR [edx]
	cmp	DWORD PTR [edi+esi*8], eax
	jmp	.L1282
.L392:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	edx, dh
	mov	eax, DWORD PTR [edi+edx*8]
	add	DWORD PTR [ecx], eax
	mov	ebx, DWORD PTR [edi+4+edx*8]
	adc	DWORD PTR [ecx+4], ebx
	jmp	.L845
.L517:
	test	al, 64
	je	.L518
	movzx	esi, cl
	mov	ecx, DWORD PTR [edx]
	sal	DWORD PTR [edi+esi*8], cl
	mov	esi, ebx
	jmp	.L845
.L367:
	test	al, 64
	je	.L368
	mov	ecx, DWORD PTR [ecx]
	movzx	esi, dl
	mov	DWORD PTR [edi+esi*8], ecx
	mov	esi, ebx
	jmp	.L845
.L415:
	test	al, 64
	je	.L416
	movzx	ebx, bl
	lea	esi, [edi+ebx*8]
	mov	eax, DWORD PTR [esi]
	imul	eax, DWORD PTR [edx]
	mov	DWORD PTR [esi], eax
	mov	esi, ecx
	jmp	.L845
.L636:
	test	al, al
	mov	esi, ecx
	jns	.L845
	mov	esi, DWORD PTR [edx]
	movzx	eax, bh
	mov	ebx, 1
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edi+4+eax*8]
	cmp	DWORD PTR [edx+4], esi
	jb	.L637
	ja	.L638
	mov	edx, DWORD PTR [ebp-48]
	cmp	edx, DWORD PTR [edi+eax*8]
	jnb	.L638
.L637:
	movzx	edx, BYTE PTR [edi+212]
	mov	esi, ecx
	and	edx, -2
	or	ebx, edx
	mov	BYTE PTR [edi+212], bl
	jmp	.L845
.L624:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, bl
	mov	ebx, 1
	mov	esi, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edx+4]
	cmp	DWORD PTR [edi+4+eax*8], esi
	jb	.L637
	ja	.L638
	mov	eax, DWORD PTR [ebp-48]
	cmp	eax, DWORD PTR [edx]
	jb	.L637
.L638:
	xor	ebx, ebx
	jmp	.L637
.L424:
	test	al, al
	mov	esi, edx
	jns	.L845
	movzx	ebx, bh
	mov	eax, DWORD PTR [ecx+4]
	imul	eax, DWORD PTR [edi+ebx*8]
	mov	edx, eax
	mov	eax, DWORD PTR [edi+4+ebx*8]
	imul	eax, DWORD PTR [ecx]
	add	eax, edx
	mov	DWORD PTR [ebp-48], eax
	mov	eax, DWORD PTR [edi+ebx*8]
	mul	DWORD PTR [ecx]
	add	edx, DWORD PTR [ebp-48]
	mov	DWORD PTR [ecx], eax
	mov	DWORD PTR [ecx+4], edx
	jmp	.L845
.L571:
	test	al, 64
	je	.L572
	movzx	esi, bl
	mov	eax, DWORD PTR [edx]
	cmp	DWORD PTR [edi+esi*8], eax
	jmp	.L1276
.L408:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	edx, dh
	mov	eax, DWORD PTR [edi+edx*8]
	sub	DWORD PTR [ecx], eax
	mov	ebx, DWORD PTR [edi+4+edx*8]
	sbb	DWORD PTR [ecx+4], ebx
	jmp	.L845
.L383:
	test	al, 64
	je	.L384
	movzx	esi, dl
	mov	ebx, DWORD PTR [ebx]
	add	DWORD PTR [edi+esi*8], ebx
	mov	esi, ecx
	jmp	.L845
.L472:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	edx, dh
	mov	eax, DWORD PTR [edi+edx*8]
	mov	ebx, DWORD PTR [edi+4+edx*8]
	and	DWORD PTR [ecx], eax
	and	DWORD PTR [ecx+4], ebx
	jmp	.L845
.L662:
	test	al, al
	mov	esi, ecx
	jns	.L845
	mov	esi, DWORD PTR [edx]
	movzx	eax, bh
	mov	ebx, 1
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edi+4+eax*8]
	cmp	DWORD PTR [edx+4], esi
	ja	.L663
	jb	.L664
	mov	edx, DWORD PTR [ebp-48]
	cmp	edx, DWORD PTR [edi+eax*8]
	jbe	.L664
.L663:
	movzx	edx, BYTE PTR [edi+212]
	mov	esi, ecx
	and	edx, -2
	or	ebx, edx
	mov	BYTE PTR [edi+212], bl
	jmp	.L845
.L650:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, bl
	mov	ebx, 1
	mov	esi, DWORD PTR [edi+eax*8]
	mov	DWORD PTR [ebp-48], esi
	mov	esi, DWORD PTR [edx+4]
	cmp	DWORD PTR [edi+4+eax*8], esi
	ja	.L663
	jb	.L664
	mov	eax, DWORD PTR [ebp-48]
	cmp	eax, DWORD PTR [edx]
	ja	.L663
.L664:
	xor	ebx, ebx
	jmp	.L663
.L456:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, dh
	push	DWORD PTR [edi+4+eax*8]
	push	DWORD PTR [edi+eax*8]
	push	DWORD PTR [ebx+4]
	push	DWORD PTR [ebx]
	call	__umoddi3
	add	esp, 16
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L845
.L703:
	test	al, al
	mov	esi, ecx
	jns	.L845
	add	esi, DWORD PTR [edx]
	jmp	.L845
.L368:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, dl
	mov	ebx, DWORD PTR [ecx]
	mov	edx, DWORD PTR [ecx+4]
	mov	DWORD PTR [edi+eax*8], ebx
	mov	DWORD PTR [edi+4+eax*8], edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L480:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, dl
	mov	edx, DWORD PTR [ecx]
	mov	ecx, DWORD PTR [ecx+4]
	lea	ebx, [edi+eax*8]
	or	DWORD PTR [ebx], edx
	or	DWORD PTR [ebx+4], ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L416:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	ebx, bl
	mov	eax, DWORD PTR [edx+4]
	lea	ebx, [edi+ebx*8]
	imul	eax, DWORD PTR [ebx]
	mov	ecx, DWORD PTR [ebx+4]
	imul	ecx, DWORD PTR [edx]
	add	ecx, eax
	mov	eax, DWORD PTR [edx]
	mul	DWORD PTR [ebx]
	add	edx, ecx
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L384:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	ecx, dl
	mov	eax, DWORD PTR [ebx]
	mov	edx, DWORD PTR [ebx+4]
	add	DWORD PTR [edi+ecx*8], eax
	adc	DWORD PTR [edi+4+ecx*8], edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L672:
	test	al, al
	mov	esi, ebx
	movzx	edx, dl
	jns	.L845
	jmp	.L1284
.L518:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, cl
	mov	ecx, DWORD PTR [edx]
	lea	ebx, [edi+eax*8]
	jmp	.L1273
	.p2align 4,,10
	.p2align 3
.L464:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, dl
	mov	edx, DWORD PTR [ecx]
	mov	ecx, DWORD PTR [ecx+4]
	lea	ebx, [edi+eax*8]
	and	DWORD PTR [ebx], edx
	and	DWORD PTR [ebx+4], ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L534:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, cl
	mov	ecx, DWORD PTR [edx]
	lea	ebx, [edi+eax*8]
	jmp	.L1274
	.p2align 4,,10
	.p2align 3
.L432:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, bl
	push	DWORD PTR [edx+4]
	push	DWORD PTR [edx]
	lea	ebx, [edi+eax*8]
	push	DWORD PTR [ebx+4]
	push	DWORD PTR [ebx]
	call	__udivdi3
	add	esp, 16
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L688:
	test	al, al
	mov	esi, ebx
	movzx	edx, dl
	jns	.L845
	jmp	.L1288
.L496:
	test	al, al
	mov	esi, ebx
	jns	.L845
	movzx	eax, dl
	mov	edx, DWORD PTR [ecx]
	mov	ecx, DWORD PTR [ecx+4]
	lea	ebx, [edi+eax*8]
	xor	DWORD PTR [ebx], edx
	xor	DWORD PTR [ebx+4], ecx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L448:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	eax, bl
	push	DWORD PTR [edx+4]
	push	DWORD PTR [edx]
	lea	ebx, [edi+eax*8]
	push	DWORD PTR [ebx+4]
	push	DWORD PTR [ebx]
	call	__umoddi3
	add	esp, 16
	mov	DWORD PTR [ebx], eax
	mov	DWORD PTR [ebx+4], edx
	jmp	.L845
	.p2align 4,,10
	.p2align 3
.L400:
	test	al, al
	mov	esi, ecx
	jns	.L845
	movzx	ecx, dl
	mov	eax, DWORD PTR [ebx]
	mov	edx, DWORD PTR [ebx+4]
	sub	DWORD PTR [edi+ecx*8], eax
	sbb	DWORD PTR [edi+4+ecx*8], edx
	jmp	.L845
.L1373:
	call	__stack_chk_fail
	.cfi_endproc
.LFE58:
	.size	Tagha_Exec, .-Tagha_Exec
	.section	.rodata.str1.1
.LC7:
	.string	"main"
	.text
	.p2align 4,,15
	.globl	Tagha_RunScript
	.type	Tagha_RunScript, @function
Tagha_RunScript:
.LFB59:
	.cfi_startproc
	push	ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	mov	ebp, esp
	.cfi_def_cfa_register 5
	push	edi
	push	esi
	push	ebx
	sub	esp, 60
	.cfi_offset 7, -12
	.cfi_offset 6, -16
	.cfi_offset 3, -20
	mov	eax, DWORD PTR [ebp+8]
	mov	ebx, DWORD PTR [ebp+16]
	mov	edx, DWORD PTR gs:20
	mov	DWORD PTR [ebp-28], edx
	xor	edx, edx
	test	eax, eax
	mov	DWORD PTR [ebp-60], eax
	je	.L1442
	mov	edx, DWORD PTR [eax+200]
	test	edx, edx
	mov	DWORD PTR [ebp-64], edx
	je	.L1430
	cmp	WORD PTR [edx], -16162
	jne	.L1430
	mov	edi, DWORD PTR [ebp-64]
	mov	edx, DWORD PTR [edi+11]
	mov	ecx, edi
	add	ecx, 15
	test	edx, edx
	mov	DWORD PTR [ebp-56], edx
	je	.L1432
	mov	eax, DWORD PTR [ecx+1]
	mov	ecx, DWORD PTR [ecx+5]
	sub	edx, 1
	add	edi, 24
	mov	esi, OFFSET FLAT:.LC7
	and	edx, 3
	mov	DWORD PTR [ebp-48], edi
	mov	DWORD PTR [ebp-68], ecx
	mov	ecx, 5
	mov	DWORD PTR [ebp-52], eax
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L1513
	mov	eax, DWORD PTR [ebp-68]
	add	eax, DWORD PTR [ebp-52]
	add	eax, DWORD PTR [ebp-48]
	cmp	DWORD PTR [ebp-56], 1
	mov	DWORD PTR [ebp-44], 1
	je	.L1432
	test	edx, edx
	je	.L1519
	cmp	edx, 1
	je	.L1498
	cmp	edx, 2
	je	.L1499
	lea	edi, [eax+9]
	mov	edx, DWORD PTR [eax+1]
	mov	ecx, 5
	mov	esi, OFFSET FLAT:.LC7
	mov	eax, DWORD PTR [eax+5]
	mov	DWORD PTR [ebp-48], edi
	repz cmpsb
	mov	DWORD PTR [ebp-52], edx
	setb	cl
	seta	dl
	add	eax, DWORD PTR [ebp-52]
	add	DWORD PTR [ebp-44], 1
	add	eax, DWORD PTR [ebp-48]
	cmp	dl, cl
	je	.L1513
.L1499:
	mov	esi, DWORD PTR [eax+1]
	lea	edi, [eax+9]
	mov	ecx, 5
	mov	eax, DWORD PTR [eax+5]
	mov	DWORD PTR [ebp-48], edi
	mov	DWORD PTR [ebp-52], esi
	mov	esi, OFFSET FLAT:.LC7
	repz cmpsb
	seta	dl
	setb	cl
	add	eax, DWORD PTR [ebp-52]
	add	DWORD PTR [ebp-44], 1
	add	eax, DWORD PTR [ebp-48]
	cmp	dl, cl
	je	.L1513
.L1498:
	mov	esi, DWORD PTR [eax+1]
	lea	edi, [eax+9]
	mov	ecx, 5
	mov	eax, DWORD PTR [eax+5]
	mov	DWORD PTR [ebp-48], edi
	mov	DWORD PTR [ebp-52], esi
	mov	esi, OFFSET FLAT:.LC7
	repz cmpsb
	je	.L1513
	add	DWORD PTR [ebp-44], 1
	add	eax, DWORD PTR [ebp-52]
	mov	edx, DWORD PTR [ebp-44]
	add	eax, DWORD PTR [ebp-48]
	cmp	DWORD PTR [ebp-56], edx
	je	.L1432
.L1519:
	mov	DWORD PTR [ebp-68], ebx
.L1435:
	lea	edx, [eax+9]
	mov	ecx, 5
	mov	ebx, DWORD PTR [eax+1]
	mov	esi, OFFSET FLAT:.LC7
	mov	eax, DWORD PTR [eax+5]
	mov	edi, edx
	repz cmpsb
	mov	DWORD PTR [ebp-48], eax
	setb	cl
	seta	al
	cmp	al, cl
	je	.L1521
	mov	esi, DWORD PTR [ebp-48]
	mov	ecx, 5
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC7
	mov	DWORD PTR [ebp-48], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L1521
	mov	esi, DWORD PTR [ebp-48]
	mov	ecx, 5
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC7
	mov	DWORD PTR [ebp-48], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L1521
	mov	esi, DWORD PTR [ebp-48]
	mov	ecx, 5
	add	esi, ebx
	add	esi, edx
	mov	edi, DWORD PTR [esi+5]
	lea	edx, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, OFFSET FLAT:.LC7
	mov	DWORD PTR [ebp-48], edi
	mov	edi, edx
	repz cmpsb
	seta	al
	setb	cl
	cmp	al, cl
	je	.L1521
	mov	eax, DWORD PTR [ebp-48]
	add	DWORD PTR [ebp-44], 4
	mov	esi, DWORD PTR [ebp-44]
	add	eax, ebx
	add	eax, edx
	cmp	DWORD PTR [ebp-56], esi
	jne	.L1435
.L1432:
	mov	edi, DWORD PTR [ebp-60]
	mov	eax, -1
	mov	DWORD PTR [edi+208], 2
	jmp	.L1428
	.p2align 4,,10
	.p2align 3
.L1521:
	mov	DWORD PTR [ebp-52], ebx
	mov	ebx, DWORD PTR [ebp-68]
	mov	DWORD PTR [ebp-48], edx
.L1513:
	mov	edx, DWORD PTR [ebp-48]
	add	edx, DWORD PTR [ebp-52]
	mov	DWORD PTR [ebp-44], edx
	je	.L1432
	mov	edi, DWORD PTR [ebp+12]
	mov	esi, DWORD PTR [ebp+12]
	lea	ecx, [26+edi*8]
	and	ecx, -16
	sub	esp, ecx
	test	ebx, ebx
	mov	edx, esp
	mov	DWORD PTR [esp+esi*8], 0
	je	.L1439
	mov	eax, DWORD PTR [ebp+12]
	test	eax, eax
	jle	.L1439
	mov	edi, DWORD PTR [ebp+12]
	mov	esi, DWORD PTR [ebx]
	mov	eax, 1
	lea	ecx, [edi-1]
	mov	DWORD PTR [esp], esi
	and	ecx, 7
	cmp	DWORD PTR [ebp+12], 1
	je	.L1439
	test	ecx, ecx
	je	.L1440
	cmp	ecx, 1
	je	.L1491
	cmp	ecx, 2
	je	.L1492
	cmp	ecx, 3
	je	.L1493
	cmp	ecx, 4
	je	.L1494
	cmp	ecx, 5
	je	.L1495
	cmp	ecx, 6
	je	.L1496
	mov	edi, DWORD PTR [ebx+eax*4]
	mov	DWORD PTR [esp+eax*8], edi
	add	eax, 1
.L1496:
	mov	ecx, DWORD PTR [ebx+eax*4]
	mov	DWORD PTR [edx+eax*8], ecx
	add	eax, 1
.L1495:
	mov	esi, DWORD PTR [ebx+eax*4]
	mov	DWORD PTR [edx+eax*8], esi
	add	eax, 1
.L1494:
	mov	edi, DWORD PTR [ebx+eax*4]
	mov	DWORD PTR [edx+eax*8], edi
	add	eax, 1
.L1493:
	mov	ecx, DWORD PTR [ebx+eax*4]
	mov	DWORD PTR [edx+eax*8], ecx
	add	eax, 1
.L1492:
	mov	esi, DWORD PTR [ebx+eax*4]
	mov	DWORD PTR [edx+eax*8], esi
	add	eax, 1
.L1491:
	mov	edi, DWORD PTR [ebx+eax*4]
	mov	DWORD PTR [edx+eax*8], edi
	add	eax, 1
	cmp	DWORD PTR [ebp+12], eax
	je	.L1439
.L1440:
	lea	edi, [eax+1]
	mov	ecx, DWORD PTR [ebx+eax*4]
	mov	esi, DWORD PTR [ebx+edi*4]
	mov	DWORD PTR [edx+eax*8], ecx
	lea	ecx, [eax+2]
	mov	DWORD PTR [edx+edi*8], esi
	mov	edi, DWORD PTR [ebx+ecx*4]
	mov	DWORD PTR [edx+ecx*8], edi
	lea	ecx, [eax+3]
	lea	edi, [eax+4]
	mov	esi, DWORD PTR [ebx+ecx*4]
	mov	DWORD PTR [edx+ecx*8], esi
	mov	ecx, DWORD PTR [ebx+edi*4]
	mov	DWORD PTR [edx+edi*8], ecx
	lea	edi, [eax+5]
	lea	ecx, [eax+6]
	mov	esi, DWORD PTR [ebx+edi*4]
	mov	DWORD PTR [edx+edi*8], esi
	mov	edi, DWORD PTR [ebx+ecx*4]
	mov	DWORD PTR [edx+ecx*8], edi
	lea	ecx, [eax+7]
	add	eax, 8
	cmp	DWORD PTR [ebp+12], eax
	mov	esi, DWORD PTR [ebx+ecx*4]
	mov	DWORD PTR [edx+ecx*8], esi
	jne	.L1440
	.p2align 4,,10
	.p2align 3
.L1439:
	mov	eax, DWORD PTR [ebp-60]
	mov	ebx, DWORD PTR [ebp+12]
	mov	DWORD PTR [eax+120], edx
	mov	edx, DWORD PTR [ebp-64]
	mov	DWORD PTR [eax+112], ebx
	mov	edi, DWORD PTR [edx+2]
	test	edi, edi
	je	.L1524
	lea	edi, [8+edi*8]
	mov	esi, esp
	lea	ecx, [edi+18]
	lea	eax, [edi-7]
	and	ecx, -16
	sub	esp, ecx
	mov	ebx, esp
	sub	esp, 4
	push	eax
	push	0
	push	ebx
	call	memset
	lea	ecx, [ebx-8+edi]
	mov	edi, DWORD PTR [ebp-60]
	mov	DWORD PTR [ecx-8], -1
	mov	DWORD PTR [ecx-4], -1
	lea	ebx, [ecx-16]
	mov	DWORD PTR [edi+184], ecx
	mov	edx, DWORD PTR [edi+188]
	mov	eax, DWORD PTR [edi+184]
	mov	DWORD PTR [edi+176], ebx
	mov	DWORD PTR [ecx-12], edx
	mov	DWORD PTR [ecx-16], eax
	mov	ecx, DWORD PTR [ebp-44]
	mov	DWORD PTR [edi+192], ecx
	mov	DWORD PTR [esp], edi
	call	Tagha_Exec
	mov	esp, esi
.L1428:
	mov	ebx, DWORD PTR [ebp-28]
	xor	ebx, DWORD PTR gs:20
	jne	.L1525
	lea	esp, [ebp-12]
	pop	ebx
	.cfi_remember_state
	.cfi_restore 3
	pop	esi
	.cfi_restore 6
	pop	edi
	.cfi_restore 7
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.p2align 4,,10
	.p2align 3
.L1430:
	.cfi_restore_state
	mov	ebx, DWORD PTR [ebp-60]
	mov	eax, -1
	mov	DWORD PTR [ebx+208], 4
	jmp	.L1428
	.p2align 4,,10
	.p2align 3
.L1524:
	mov	esi, DWORD PTR [ebp-60]
	mov	eax, -1
	mov	DWORD PTR [esi+208], 5
	jmp	.L1428
.L1442:
	mov	eax, -1
	jmp	.L1428
.L1525:
	call	__stack_chk_fail
	.cfi_endproc
.LFE59:
	.size	Tagha_RunScript, .-Tagha_RunScript
	.p2align 4,,15
	.globl	Tagha_CallFunc
	.type	Tagha_CallFunc, @function
Tagha_CallFunc:
.LFB60:
	.cfi_startproc
	push	ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	mov	ebp, esp
	.cfi_def_cfa_register 5
	push	edi
	push	esi
	push	ebx
	sub	esp, 60
	.cfi_offset 7, -12
	.cfi_offset 6, -16
	.cfi_offset 3, -20
	mov	eax, DWORD PTR [ebp+8]
	mov	ebx, DWORD PTR [ebp+12]
	mov	edi, DWORD PTR [ebp+20]
	mov	edx, DWORD PTR gs:20
	mov	DWORD PTR [ebp-28], edx
	xor	edx, edx
	test	eax, eax
	mov	DWORD PTR [ebp-52], eax
	mov	DWORD PTR [ebp-44], ebx
	mov	DWORD PTR [ebp-64], edi
	je	.L1543
	test	ebx, ebx
	je	.L1543
	mov	edx, DWORD PTR [eax+200]
	test	edx, edx
	mov	DWORD PTR [ebp-60], edx
	je	.L1528
	cmp	WORD PTR [edx], -16162
	jne	.L1528
	mov	esi, DWORD PTR [ebp-60]
	mov	ecx, DWORD PTR [esi+11]
	mov	eax, esi
	add	eax, 15
	test	ecx, ecx
	mov	DWORD PTR [ebp-56], ecx
	je	.L1530
	lea	ebx, [ecx-1]
	lea	edi, [esi+24]
	sub	esp, 8
	mov	esi, DWORD PTR [eax+5]
	and	ebx, 3
	mov	DWORD PTR [ebp-48], ebx
	mov	ebx, DWORD PTR [eax+1]
	push	edi
	push	DWORD PTR [ebp-44]
	call	strcmp
	add	esp, 16
	test	eax, eax
	mov	edx, DWORD PTR [ebp-48]
	je	.L1620
	add	esi, ebx
	mov	DWORD PTR [ebp-48], 1
	add	esi, edi
	cmp	DWORD PTR [ebp-56], 1
	je	.L1530
	test	edx, edx
	je	.L1533
	cmp	edx, 1
	je	.L1603
	cmp	edx, 2
	je	.L1604
	mov	ebx, DWORD PTR [esi+1]
	lea	edi, [esi+9]
	mov	esi, DWORD PTR [esi+5]
	sub	esp, 8
	push	edi
	push	DWORD PTR [ebp-44]
	add	esi, ebx
	call	strcmp
	add	esi, edi
	add	esp, 16
	add	DWORD PTR [ebp-48], 1
	test	eax, eax
	je	.L1620
.L1604:
	mov	ebx, DWORD PTR [esi+1]
	lea	edi, [esi+9]
	sub	esp, 8
	mov	esi, DWORD PTR [esi+5]
	push	edi
	push	DWORD PTR [ebp-44]
	add	esi, ebx
	call	strcmp
	add	esi, edi
	add	esp, 16
	add	DWORD PTR [ebp-48], 1
	test	eax, eax
	je	.L1620
.L1603:
	lea	edi, [esi+9]
	sub	esp, 8
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	push	DWORD PTR [ebp-44]
	call	strcmp
	add	esp, 16
	test	eax, eax
	je	.L1620
	add	DWORD PTR [ebp-48], 1
	add	esi, ebx
	mov	eax, DWORD PTR [ebp-48]
	add	esi, edi
	cmp	DWORD PTR [ebp-56], eax
	je	.L1530
.L1533:
	lea	edi, [esi+9]
	sub	esp, 8
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	push	DWORD PTR [ebp-44]
	call	strcmp
	add	esp, 16
	test	eax, eax
	je	.L1620
	add	esi, ebx
	sub	esp, 8
	add	esi, edi
	lea	edi, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	push	DWORD PTR [ebp-44]
	call	strcmp
	add	esp, 16
	test	eax, eax
	je	.L1620
	add	esi, ebx
	sub	esp, 8
	add	esi, edi
	lea	edi, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	push	DWORD PTR [ebp-44]
	call	strcmp
	add	esp, 16
	test	eax, eax
	je	.L1620
	add	esi, ebx
	sub	esp, 8
	add	esi, edi
	lea	edi, [esi+9]
	mov	ebx, DWORD PTR [esi+1]
	mov	esi, DWORD PTR [esi+5]
	push	edi
	push	DWORD PTR [ebp-44]
	call	strcmp
	add	esp, 16
	test	eax, eax
	je	.L1620
	add	DWORD PTR [ebp-48], 4
	add	esi, ebx
	mov	eax, DWORD PTR [ebp-48]
	add	esi, edi
	cmp	DWORD PTR [ebp-56], eax
	jne	.L1533
.L1530:
	mov	ecx, DWORD PTR [ebp-52]
	mov	eax, -1
	mov	DWORD PTR [ecx+208], 2
	jmp	.L1526
	.p2align 4,,10
	.p2align 3
.L1620:
	add	ebx, edi
	je	.L1530
	mov	ecx, DWORD PTR [ebp-60]
	mov	edx, DWORD PTR [ecx+2]
	test	edx, edx
	je	.L1630
	lea	esi, [8+edx*8]
	lea	eax, [esi+18]
	lea	ecx, [esi-7]
	and	eax, -16
	sub	esp, eax
	mov	edi, esp
	sub	esp, 4
	push	ecx
	push	0
	lea	esi, [edi-8+esi]
	push	edi
	call	memset
	add	esp, 16
	cmp	DWORD PTR [ebp+16], 8
	mov	edx, DWORD PTR [ebp-52]
	mov	DWORD PTR [edx+184], esi
	mov	edi, edx
	mov	DWORD PTR [edx+176], esi
	ja	.L1535
	movzx	eax, WORD PTR [ebp+16]
	lea	edx, [edx+112]
	sal	eax, 3
	movzx	eax, ax
	cmp	eax, 4
	jb	.L1631
	mov	edi, DWORD PTR [ebp-64]
	mov	esi, DWORD PTR [ebp-52]
	mov	ecx, DWORD PTR [edi]
	mov	DWORD PTR [esi+112], ecx
	mov	ecx, DWORD PTR [edi-4+eax]
	mov	DWORD PTR [edx-4+eax], ecx
	lea	ecx, [esi+116]
	and	ecx, -4
	sub	edx, ecx
	add	eax, edx
	sub	edi, edx
	and	eax, -4
	cmp	eax, 4
	jb	.L1537
	mov	edx, DWORD PTR [edi]
	lea	esi, [eax-1]
	shr	esi, 2
	mov	DWORD PTR [ecx], edx
	mov	edx, 4
	and	esi, 7
	cmp	edx, eax
	jnb	.L1537
	test	esi, esi
	je	.L1539
	cmp	esi, 1
	je	.L1596
	cmp	esi, 2
	je	.L1597
	cmp	esi, 3
	je	.L1598
	cmp	esi, 4
	je	.L1599
	cmp	esi, 5
	je	.L1600
	cmp	esi, 6
	je	.L1601
	mov	esi, DWORD PTR [edi+4]
	mov	edx, 8
	mov	DWORD PTR [ecx+4], esi
.L1601:
	mov	esi, DWORD PTR [edi+edx]
	mov	DWORD PTR [ecx+edx], esi
	add	edx, 4
.L1600:
	mov	esi, DWORD PTR [edi+edx]
	mov	DWORD PTR [ecx+edx], esi
	add	edx, 4
.L1599:
	mov	esi, DWORD PTR [edi+edx]
	mov	DWORD PTR [ecx+edx], esi
	add	edx, 4
.L1598:
	mov	esi, DWORD PTR [edi+edx]
	mov	DWORD PTR [ecx+edx], esi
	add	edx, 4
.L1597:
	mov	esi, DWORD PTR [edi+edx]
	mov	DWORD PTR [ecx+edx], esi
	add	edx, 4
.L1596:
	mov	esi, DWORD PTR [edi+edx]
	mov	DWORD PTR [ecx+edx], esi
	add	edx, 4
	cmp	edx, eax
	jnb	.L1537
.L1539:
	mov	esi, DWORD PTR [edi+edx]
	mov	DWORD PTR [ecx+edx], esi
	mov	esi, DWORD PTR [edi+4+edx]
	mov	DWORD PTR [ecx+4+edx], esi
	mov	esi, DWORD PTR [edi+8+edx]
	mov	DWORD PTR [ecx+8+edx], esi
	mov	esi, DWORD PTR [edi+12+edx]
	mov	DWORD PTR [ecx+12+edx], esi
	mov	esi, DWORD PTR [edi+16+edx]
	mov	DWORD PTR [ecx+16+edx], esi
	mov	esi, DWORD PTR [edi+20+edx]
	mov	DWORD PTR [ecx+20+edx], esi
	mov	esi, DWORD PTR [edi+24+edx]
	mov	DWORD PTR [ecx+24+edx], esi
	mov	esi, DWORD PTR [edi+28+edx]
	mov	DWORD PTR [ecx+28+edx], esi
	add	edx, 32
	cmp	edx, eax
	jb	.L1539
	jmp	.L1537
	.p2align 4,,10
	.p2align 3
.L1535:
	mov	ecx, DWORD PTR [ebp-64]
	mov	edi, DWORD PTR [ebp-52]
	sub	esp, 4
	mov	edx, DWORD PTR [ecx]
	mov	eax, edi
	lea	ecx, [ecx+64]
	mov	DWORD PTR [edi+112], edx
	mov	edi, DWORD PTR [ecx-60]
	mov	DWORD PTR [eax+116], edi
	mov	edx, DWORD PTR [ecx-56]
	mov	DWORD PTR [eax+120], edx
	mov	edi, DWORD PTR [ecx-52]
	mov	DWORD PTR [eax+124], edi
	mov	edx, DWORD PTR [ecx-48]
	mov	DWORD PTR [eax+128], edx
	mov	edi, DWORD PTR [ecx-44]
	mov	DWORD PTR [eax+132], edi
	mov	edx, DWORD PTR [ecx-40]
	mov	DWORD PTR [eax+136], edx
	mov	edi, DWORD PTR [ecx-36]
	mov	DWORD PTR [eax+140], edi
	mov	edx, DWORD PTR [ecx-32]
	mov	DWORD PTR [eax+144], edx
	mov	edi, DWORD PTR [ecx-28]
	mov	DWORD PTR [eax+148], edi
	mov	edx, DWORD PTR [ecx-24]
	mov	DWORD PTR [eax+152], edx
	mov	edi, DWORD PTR [ecx-20]
	mov	DWORD PTR [eax+156], edi
	mov	edx, DWORD PTR [ecx-16]
	mov	DWORD PTR [eax+160], edx
	mov	edi, DWORD PTR [ecx-12]
	mov	DWORD PTR [eax+164], edi
	mov	edx, DWORD PTR [ecx-8]
	mov	DWORD PTR [eax+168], edx
	mov	edi, DWORD PTR [ecx-4]
	mov	DWORD PTR [eax+172], edi
	mov	eax, DWORD PTR [ebp+16]
	lea	edi, [-64+eax*8]
	push	edi
	push	ecx
	push	esi
	sub	esi, edi
	call	memcpy
	add	esp, 16
.L1541:
	mov	ecx, DWORD PTR [ebp-52]
	lea	eax, [esi-16]
	sub	esp, 12
	mov	edi, DWORD PTR [ecx+192]
	mov	edx, DWORD PTR [ecx+196]
	mov	DWORD PTR [ecx+176], eax
	mov	eax, DWORD PTR [ecx+188]
	mov	DWORD PTR [ecx+192], ebx
	mov	DWORD PTR [esi-8], edi
	mov	edi, DWORD PTR [ecx+184]
	mov	DWORD PTR [esi-4], edx
	mov	DWORD PTR [esi-12], eax
	mov	DWORD PTR [esi-16], edi
	push	ecx
	call	Tagha_Exec
	add	esp, 16
.L1526:
	mov	ebx, DWORD PTR [ebp-28]
	xor	ebx, DWORD PTR gs:20
	jne	.L1632
	lea	esp, [ebp-12]
	pop	ebx
	.cfi_remember_state
	.cfi_restore 3
	pop	esi
	.cfi_restore 6
	pop	edi
	.cfi_restore 7
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.p2align 4,,10
	.p2align 3
.L1631:
	.cfi_restore_state
	test	eax, eax
	je	.L1537
	mov	ecx, DWORD PTR [ebp-64]
	movzx	edx, BYTE PTR [ecx]
	mov	BYTE PTR [edi+112], dl
.L1537:
	mov	edi, DWORD PTR [ebp-52]
	mov	esi, DWORD PTR [edi+176]
	jmp	.L1541
	.p2align 4,,10
	.p2align 3
.L1528:
	mov	edx, DWORD PTR [ebp-52]
	mov	eax, -1
	mov	DWORD PTR [edx+208], 4
	jmp	.L1526
	.p2align 4,,10
	.p2align 3
.L1630:
	mov	ebx, DWORD PTR [ebp-52]
	mov	eax, -1
	mov	DWORD PTR [ebx+208], 5
	jmp	.L1526
	.p2align 4,,10
	.p2align 3
.L1543:
	mov	eax, -1
	jmp	.L1526
.L1632:
	call	__stack_chk_fail
	.cfi_endproc
.LFE60:
	.size	Tagha_CallFunc, .-Tagha_CallFunc
	.p2align 4,,15
	.globl	Tagha_GetReturnValue
	.type	Tagha_GetReturnValue, @function
Tagha_GetReturnValue:
.LFB61:
	.cfi_startproc
	sub	esp, 28
	.cfi_def_cfa_offset 32
	mov	edx, DWORD PTR [esp+36]
	mov	eax, DWORD PTR [esp+32]
	mov	ecx, DWORD PTR gs:20
	mov	DWORD PTR [esp+12], ecx
	xor	ecx, ecx
	test	edx, edx
	je	.L1634
	mov	ecx, DWORD PTR [edx+4]
	mov	edx, DWORD PTR [edx]
	mov	DWORD PTR [eax+4], ecx
	mov	DWORD PTR [eax], edx
.L1633:
	mov	ecx, DWORD PTR [esp+12]
	xor	ecx, DWORD PTR gs:20
	jne	.L1638
	add	esp, 28
	.cfi_remember_state
	.cfi_def_cfa_offset 4
	ret	4
	.p2align 4,,10
	.p2align 3
.L1634:
	.cfi_restore_state
	mov	DWORD PTR [eax], 0
	mov	DWORD PTR [eax+4], 0
	jmp	.L1633
.L1638:
	call	__stack_chk_fail
	.cfi_endproc
.LFE61:
	.size	Tagha_GetReturnValue, .-Tagha_GetReturnValue
	.p2align 4,,15
	.globl	Tagha_GetGlobalVarByName
	.type	Tagha_GetGlobalVarByName, @function
Tagha_GetGlobalVarByName:
.LFB62:
	.cfi_startproc
	push	ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	push	edi
	.cfi_def_cfa_offset 12
	.cfi_offset 7, -12
	push	esi
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	push	ebx
	.cfi_def_cfa_offset 20
	.cfi_offset 3, -20
	sub	esp, 28
	.cfi_def_cfa_offset 48
	mov	eax, DWORD PTR [esp+48]
	mov	esi, DWORD PTR [esp+52]
	test	eax, eax
	je	.L1645
	test	esi, esi
	je	.L1645
	mov	ecx, DWORD PTR [eax+200]
	test	ecx, ecx
	je	.L1645
	mov	edx, DWORD PTR [ecx+7]
	lea	eax, [ecx+11+edx]
	mov	edi, DWORD PTR [eax]
	test	edi, edi
	mov	DWORD PTR [esp+4], edi
	je	.L1645
	mov	ecx, DWORD PTR [eax+9]
	lea	ebx, [edi-1]
	lea	ebp, [eax+13]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	and	ebx, 3
	mov	DWORD PTR [esp+16], ebx
	mov	DWORD PTR [esp+20], ecx
	mov	ebx, DWORD PTR [eax+5]
	push	ebp
	.cfi_def_cfa_offset 60
	push	esi
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	mov	edx, DWORD PTR [esp+8]
	je	.L1676
	add	ebx, DWORD PTR [esp+12]
	cmp	DWORD PTR [esp+4], 1
	mov	edi, 1
	lea	eax, [ebp+0+ebx]
	je	.L1645
	test	edx, edx
	je	.L1642
	cmp	edx, 1
	je	.L1669
	cmp	edx, 2
	je	.L1670
	mov	ebx, DWORD PTR [eax+1]
	lea	ebp, [eax+9]
	mov	eax, DWORD PTR [eax+5]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	add	edi, 1
	mov	DWORD PTR [esp+16], eax
	push	ebp
	.cfi_def_cfa_offset 60
	push	esi
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	mov	ecx, eax
	mov	eax, DWORD PTR [esp+8]
	add	eax, ebx
	add	eax, ebp
	test	ecx, ecx
	je	.L1676
.L1670:
	mov	edx, DWORD PTR [eax+5]
	lea	ebp, [eax+9]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	mov	ebx, DWORD PTR [eax+1]
	add	edi, 1
	mov	DWORD PTR [esp+16], edx
	push	ebp
	.cfi_def_cfa_offset 60
	push	esi
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	mov	ecx, eax
	mov	eax, DWORD PTR [esp+8]
	add	eax, ebx
	add	eax, ebp
	test	ecx, ecx
	je	.L1676
.L1669:
	mov	ebx, DWORD PTR [eax+1]
	lea	ebp, [eax+9]
	mov	eax, DWORD PTR [eax+5]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	mov	DWORD PTR [esp+16], eax
	push	ebp
	.cfi_def_cfa_offset 60
	push	esi
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L1676
	add	ebx, DWORD PTR [esp+8]
	add	edi, 1
	cmp	DWORD PTR [esp+4], edi
	lea	eax, [ebp+0+ebx]
	je	.L1645
.L1642:
	mov	edx, DWORD PTR [eax+5]
	lea	ebp, [eax+9]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	mov	ebx, DWORD PTR [eax+1]
	mov	DWORD PTR [esp+16], edx
	push	ebp
	.cfi_def_cfa_offset 60
	push	esi
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L1676
	add	ebx, DWORD PTR [esp+8]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	lea	ecx, [ebp+0+ebx]
	mov	eax, DWORD PTR [ecx+5]
	lea	ebp, [ecx+9]
	mov	ebx, DWORD PTR [ecx+1]
	mov	DWORD PTR [esp+16], eax
	push	ebp
	.cfi_def_cfa_offset 60
	push	esi
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L1676
	add	ebx, DWORD PTR [esp+8]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	lea	edx, [ebp+0+ebx]
	mov	ecx, DWORD PTR [edx+5]
	lea	ebp, [edx+9]
	mov	ebx, DWORD PTR [edx+1]
	mov	DWORD PTR [esp+16], ecx
	push	ebp
	.cfi_def_cfa_offset 60
	push	esi
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L1676
	add	ebx, DWORD PTR [esp+8]
	sub	esp, 8
	.cfi_def_cfa_offset 56
	lea	eax, [ebp+0+ebx]
	mov	edx, DWORD PTR [eax+5]
	lea	ebp, [eax+9]
	mov	ebx, DWORD PTR [eax+1]
	mov	DWORD PTR [esp+16], edx
	push	ebp
	.cfi_def_cfa_offset 60
	push	esi
	.cfi_def_cfa_offset 64
	call	strcmp
	add	esp, 16
	.cfi_def_cfa_offset 48
	test	eax, eax
	je	.L1676
	add	ebx, DWORD PTR [esp+8]
	add	edi, 4
	cmp	DWORD PTR [esp+4], edi
	lea	eax, [ebp+0+ebx]
	jne	.L1642
.L1645:
	add	esp, 28
	.cfi_remember_state
	.cfi_def_cfa_offset 20
	xor	eax, eax
	pop	ebx
	.cfi_restore 3
	.cfi_def_cfa_offset 16
	pop	esi
	.cfi_restore 6
	.cfi_def_cfa_offset 12
	pop	edi
	.cfi_restore 7
	.cfi_def_cfa_offset 8
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa_offset 4
	ret
	.p2align 4,,10
	.p2align 3
.L1676:
	.cfi_restore_state
	add	esp, 28
	.cfi_def_cfa_offset 20
	lea	eax, [ebp+0+ebx]
	pop	ebx
	.cfi_restore 3
	.cfi_def_cfa_offset 16
	pop	esi
	.cfi_restore 6
	.cfi_def_cfa_offset 12
	pop	edi
	.cfi_restore 7
	.cfi_def_cfa_offset 8
	pop	ebp
	.cfi_restore 5
	.cfi_def_cfa_offset 4
	ret
	.cfi_endproc
.LFE62:
	.size	Tagha_GetGlobalVarByName, .-Tagha_GetGlobalVarByName
	.section	.rodata.str1.1
.LC8:
	.string	"Null VM Pointer"
.LC9:
	.string	"Unknown Error"
	.text
	.p2align 4,,15
	.globl	Tagha_GetError
	.type	Tagha_GetError, @function
Tagha_GetError:
.LFB63:
	.cfi_startproc
	mov	eax, DWORD PTR [esp+4]
	test	eax, eax
	je	.L1681
	mov	edx, DWORD PTR [eax+208]
	mov	eax, OFFSET FLAT:.LC9
	add	edx, 1
	cmp	edx, 6
	ja	.L1679
	mov	eax, DWORD PTR CSWTCH.137[0+edx*4]
	ret
	.p2align 4,,10
	.p2align 3
.L1681:
	mov	eax, OFFSET FLAT:.LC8
.L1679:
	rep ret
	.cfi_endproc
.LFE63:
	.size	Tagha_GetError, .-Tagha_GetError
	.section	.rodata.str1.1
.LC10:
	.string	"Out of Bound Instruction"
.LC11:
	.string	"None"
.LC12:
	.string	"Null or Invalid Pointer"
.LC13:
	.string	"Missing Function"
.LC14:
	.string	"Missing Native"
.LC15:
	.string	"Null or Invalid Script"
.LC16:
	.string	"Bad Stack Size given"
	.section	.rodata
	.align 4
	.type	CSWTCH.137, @object
	.size	CSWTCH.137, 28
CSWTCH.137:
	.long	.LC10
	.long	.LC11
	.long	.LC12
	.long	.LC13
	.long	.LC14
	.long	.LC15
	.long	.LC16
	.align 32
	.type	dispatch.3852, @object
	.size	dispatch.3852, 188
dispatch.3852:
	.long	.L338
	.long	.L345
	.long	.L350
	.long	.L353
	.long	.L361
	.long	.L377
	.long	.L393
	.long	.L409
	.long	.L425
	.long	.L441
	.long	.L457
	.long	.L473
	.long	.L489
	.long	.L505
	.long	.L511
	.long	.L527
	.long	.L543
	.long	.L549
	.long	.L555
	.long	.L561
	.long	.L587
	.long	.L613
	.long	.L639
	.long	.L665
	.long	.L681
	.long	.L697
	.long	.L704
	.long	.L711
	.long	.L718
	.long	.L728
	.long	.L727
	.long	.L739
	.long	.L741
	.long	.L743
	.long	.L746
	.long	.L749
	.long	.L762
	.long	.L775
	.long	.L788
	.long	.L853
	.long	.L858
	.long	.L863
	.long	.L801
	.long	.L814
	.long	.L827
	.long	.L840
	.long	.L341
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC4:
	.long	1602224128
	.ident	"GCC: (Ubuntu 6.4.0-17ubuntu1~16.04) 6.4.0 20180424"
	.section	.note.GNU-stack,"",@progbits
