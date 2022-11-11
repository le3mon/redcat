; PAE 비트를 1로 설정
mov eax, cr4 ; CR4 컨트롤 레지스터의 값을 EAX 레지스터에 저장
or eax, 0x20 ; PAE 비트(비트 5)를 1로 설정
mov cr4, eax ; 설정된 값을 다시 CR4 컨트롤 레지스터에 저장
; PML4 테이블의 어드레스와 캐시 활성화
mov eax, 0x100000 ; EAX 레지스터에 PML4 테이블이 존재하는 0x100000(1MB)를 저장
mov cr3, eax ; CR3 컨트롤 레지스터에 0x100000(1MB)을 저장
; 프로세서의 페이징 기능 활성화
mov eax, cr0 ; EAX 레지스터에 CR0 컨트롤 레지스터를 저장
or eax, 0x80000000 ; PG 비트(비트 31)를 1로 설정
mov cr0, eax ; 설정된 값을 다시 CR0 컨트롤 레지스터에 저장