    .align 4
    .section .data
    .section .data.fs_img
    .global fs_img_start
    .global fs_img_end
fs_img_start:
    .incbin "./disk.img"
fs_img_end:
    .quad 0xffffffff

    

