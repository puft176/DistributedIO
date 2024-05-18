#ifndef __IOInput_H__
#define __IOInput_H__

extern uint16_t InputIO[];
extern GPIO_TypeDef* InputType[];

void IOInput_Init(void);
void IOOutput_Init(void);

#endif

