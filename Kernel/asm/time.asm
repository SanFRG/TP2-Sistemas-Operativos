GLOBAL rtc_get_seconds
GLOBAL rtc_get_minutes
GLOBAL rtc_get_hours
GLOBAL rtc_get_day
GLOBAL rtc_get_month
GLOBAL rtc_get_year

section .text

; al = RTC register number; returns value in al
rtc_read:
    out 70h, al
    in al, 71h
    ret

rtc_get_seconds:
    mov al, 0
    call rtc_read
    ret

rtc_get_minutes:
    mov al, 2
    call rtc_read
    ret

rtc_get_hours:
    mov al, 4
    call rtc_read
    ret

rtc_get_day:
    mov al, 7
    call rtc_read
    ret

rtc_get_month:
    mov al, 8
    call rtc_read
    ret

rtc_get_year:
    mov al, 9
    call rtc_read
    ret
