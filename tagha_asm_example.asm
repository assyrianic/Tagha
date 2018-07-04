# manually set stack size
# directives start with $
# if stacksize isn't set, the default stacksize is 128 which will end up as
# 128 * 8 == 1024 bytes of stack size.
# also, the stack size will be aligned to a size of 8
# so 129 will not actually make a stack size of 129 * 8 bytes,
# 129 will be changed at runtime to 136 * 8
$stacksize 0xFF

# global var named 'i' which is 12 bytes (struct) and is assigned the values after the bytesize.
$global i, 12,	word 0, long 0
$global n, 4,	byte 0, byte 0, byte 0, byte 0x40 # alternative: long 0x40000000 or 1073741824

# global var for strings, 12 is string length WITH null terminator.
$global str 12	"hello world"
$native %printf

# uint32_t factorial(const uint32_t i)
# {
#     if( i<=1 )
#         return 1;
#     return i * factorial(i-1);
# }
%factorial:
	sub	rsp, 16 # 4+8 = 12
	mov	[long rbp-4], rsemkath # 4+1+4 = 9
	
# if( i<=1 )
	cmp	[long rbp-4], 1 # 4+1+4+8 = 17
	jz	.cont # 2+8 = 10
	
# return 1;
	mov ralaf, 1 # 4+8 = 12
	syscall %printf, 1 # 6 + 8 = 14
	ret # 2
	
.cont: # should be 61 | 75 if syscall is not commented out.
# return i * factorial(i-1);
	mov ralaf, [long rbp-4]
	sub ralaf, 1
	mov rsemkath, ralaf
	call %factorial
	mul ralaf, [long rbp-4]
	mov rsemkath, ralaf
	ret
.end:
