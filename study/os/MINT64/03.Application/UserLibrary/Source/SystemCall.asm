[BITS 64]           ; ������ �ڵ�� 64��Ʈ �ڵ�� ����

SECTION .text       ; text ����(���׸�Ʈ)�� ����

; C ���� ȣ���� �� �ֵ��� �̸��� ������(Export)
global ExecuteSystemCall

; �ý��� ���� ����
;   PARAM: QWORD qwServiceNumber, PARAMETERTABLE* pstParameter
ExecuteSystemCall:
    push rcx        ; SYSCALL�� ȣ���� �� RCX �������Ϳ� RIP �������Ͱ� ����ǰ� 
    push r11        ; R11 �������Ϳ� RFLAGS �������Ͱ� ����ǹǷ� ���ÿ� ����
    
    syscall         ; SYSCALL ����
    
    pop r11         ; ���ÿ� ����� ������ RCX �������Ϳ� R11 �������͸� ����
    pop rcx
    ret
