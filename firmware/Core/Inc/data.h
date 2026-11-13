#ifndef DATA_H
#define DATA_H

#include <stdint.h>

void     Data_Init(void);          /* dat bo dem ve 0 */
void     Data_IncrementA(void);
void     Data_IncrementB(void);
uint32_t Data_GetCountA(void);
uint32_t Data_GetCountB(void);
uint32_t Data_GetCountTotal(void);
void     Data_Reset(void);         /* chi goi o STATE_STOP */

#endif