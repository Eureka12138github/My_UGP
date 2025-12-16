#include "stm32f10x.h"                  // Device header
//extern uint16_t Num;定义外部变量？
void Timer2_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);//tIM2是APB1总线的外设，所以要开启APB1时钟
	
	TIM_InternalClockConfig(TIM2);//这句表明TIM2的时基单元由内部时钟驱动，系统默认由内部时钟驱动，不写这句也行
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;//TIM_ClockDivision与滤波的频率有关，决定是否对内部时钟进行分频，需滤波强一点，就TIM_CKD_DIV2或者TIM_CKD_DIV4
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter=0;//此处高级计数器才用到，故直接给零了
	TIM_TimeBaseInitStructure.TIM_Period=10-1;//ARR,减一是因为预分频器和计数器都有1个数的偏差
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;//这里是设置计数器的计数模式的，设置为向上计数
	TIM_TimeBaseInitStructure.TIM_Prescaler=7200-1;//这里是设置预分频器的主频的“切分”，也就是把主频“切”成7200份
	/*
	时基单元：PSC(预分频器)、ARR(自动重装器)、CNT(计数器)
	计数器溢出频率:CK_CNT_OV=CK_CNT/(ARR+1)=CK_PSC/(PSC+1)/(ARR+1)
	害，实际要计算每次计数时间就相当于1/(接到分频器上的频率(Mhz)*1000000/预分频器的值-1/重装的值-1)
	例如：现在我接到PSC上的频率为单片机的主频即72Mhz，而PSC的值为7200，即进行7200分频，重装值为10000，即
	此时根据公式可得：1/(72000000/7200-1+1/10000-1+1)=1s，此处的加一应为硬件自动加的
	PSC预分配器，对主频72Mhz进行分频，为0表示不分频，即还是72Mhz,为1表示二分频，即72Mhz/2
	故在此处表示的是进行7200分频，即72Mhz/7200，但不能直接写7200，根据其规则，应该要写7200-1
	它后面会自己“+1”从而把“-1”给抵消掉
	另：PSC和ARR都有自己的缓冲寄存器，它们的目的是为了在中断更改相应值的时候不会出现错误，用或不用，可自己设置
	具体见江协视频6-1 的39分左右
	另：PSC与ARR的取值范围是0~65535，不能超范围，若预PSC小点，ARR大点，那就是以较高的频率计较多的数
	若若预PSC大点，ARR小点，那就是以较低的频率计较少的数，但结果是不变的，例如在这里我可以
	*/
	
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//初始化通用定时器TIM2的时基单元
	
	TIM_ClearFlag(TIM2,TIM_FLAG_Update);
	/*
	加了上面这句，复位后数才从0开始，原问题来自于TIM_TimeBaseInit函数带来的副作用，
	即更新事件和更新中断是同时发生的，更新中断会置中断标志位，初始化一旦完成了，更新中断就会立刻进入
	这就是刚上电就进入中断的原因（依旧不太懂。。。）
	
	04042218的理解：这个函数的作用是将TIM2的时基单元进行初始化的，上面利用结构体已经将PSC和ARR的值进行进行重装
	但是这PSC和ARR都是有缓冲寄存器的，真正起作用的也是这两个缓冲寄存器，故想对PSC和ARR的值进行重装，就需要改变他们缓冲
	寄存器的值，而它们的缓冲寄存器只有在更新事件时才会改变，但是刚上电时并没有更新事件，所以需要手动写一个更新事件，就是
	这句“TIMx->EGR = TIM_PSCReloadMode_Immediate; ”，这样PSC和ARR的值才会在初始化的时候马上重装，没有这句的话只能等到下次
	更新事件到来时，才会真正重装，这样初始化就晚了，相当于“如初”。但人为地使更新事件来临会有一个缺点，那就是在上电初始化的
	一瞬间就会就会进入中断，因为更新事件和更新中断是同时发生的，所以每次复位的一瞬间就会进入一次中断，故在这个程序中OLED显示
	的Num一开始的值会是1而不是0，因为在复位的一瞬间已经进过一次中断了。这句“TIM_ClearFlag(TIM2,TIM_FLAG_Update);”是用来
	清除更新中断标志位的，这样在初始化时PSC和ARR的值能马上重装而又不会触发多余的一次中断了。（总算是理解了其中原理了，废了老
	大劲了，那个UP是当真厉害，这是从根本上把问题解决了，如果是我的话，可能在OLED显示时直接显示Num-1吧。。。）
	另：- **更新中断**：计数器达到自动重装载值时，触发的中断，需要通过中断控制器通知处理器。
		- **更新事件**：计数器达到自动重装载值时发生的内部事件，可以触发其他定时器操作。
		两者通常同时发生，但更新中断需要处理器的中断服务程序来处理，而更新事件可以由定时器内部逻辑处理。

	
	*/
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);//开启了更新中断到NVIC的通路，这里不太理解，为什么是更新中断？
	/*
	因为此处需要定时器触发中断而并非事件，另外这个函数是用来使能中断的，即“告诉”TIM2，你可以进行中断了
	*/
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
/*
	凡是中断都得由NVIC管理，所以这里它又出现了
	其配置都是有套路的，基本和GPIO的配置差不多，
	需要搞明白的是结构体中的各个参数，要理解它们对应的功能
	结合不同应用场景对它们进行相对应的配置

*/	
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn;//这句是找到TIM2在NVIC中的中断通道，不同类型的中断，在NVIC中的中断名称也不一
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//这句是将该通道打通
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//这句和下一句是配置优先级的为什么这样配我现在也不清楚
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	//上面优先级配置我感觉影响不大，因为只有一个中断源
	NVIC_Init(&NVIC_InitStructure);
	TIM_Cmd(TIM2,ENABLE);//这句相当于给TIM2使能，我的理解是相当于51中的ET0=1;吧，要使用定时器，这句一定得加上，否则定时器都没打开你让它怎么工作？
}
/*
定时器名称是规定好的，需要在start文件里面的mds里面找
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET)
	{
		Num++;
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	}
}

*/
