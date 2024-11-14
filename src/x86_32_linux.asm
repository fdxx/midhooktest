bits 32

sub esp, 20
push dword [data]
push dword [esp + 28]
call [destination]
add esp, 28
ret

data:
dd 0
destination:
dd 0
