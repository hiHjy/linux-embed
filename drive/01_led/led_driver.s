.global _start 

_start:
    /*--配置所有时钟使能--*/
    ldr r0, =0x020c4068 @CCM_CCGR0
    ldr r1, =0xffffffff  
    str r1, [r0]

    ldr r0, =0x020c406c @CCM_CCGR1
    str r1, [r0]

    ldr r0, =0x020c4070 @CCM_CCGR2
    str r1, [r0]

    ldr r0, =0x020c4074 @CCM_CCGR3
    str r1, [r0]

    ldr r0, =0x020c4078 @CCM_CCGR4
    str r1, [r0]

    ldr r0, =0x020c407c @CCM_CCGR5
    str r1, [r0]

    ldr r0, =0x020c4080 @CCM_CCGR6
    str r1, [r0]

/*--配置为引脚gpio-- */

    ldr r0, =0x020e0068
    ldr r1, =0x00000005      
    str r1, [r0]

/*--配置电气属性--*/
/**
    bit0: 0 低压摆率
    bit5-3: 110 驱动能力 DSE_6_R0_6 — R0/6
    bit7-6： 10 速度DSE_6_R0_6 — R0/6
    bit11: 0 不是开漏输出
    bit12： 1 启用引脚的内部上拉/下拉电阻或保持器电路（具体是上拉还是下拉由其他寄存器位决定）。
    bit13: 0  通过电阻强制拉高/拉低电平
    bit15-14:00 100k下拉
    bit16：0

 */
    ldr r0, =0x020e02f4
    ldr r1, =0x000010B0
    str r1, [r0]

/*--配置gpio功能(方向，中断？)-- */
    ldr r0, =0x209c004
    ldr r1, =0x4
    str r1, [r0]

/*--开灯-- */

    ldr r0, =0x209c000
    ldr r1, =0x0
    str r1, [r0]

loop:
    b loop
    