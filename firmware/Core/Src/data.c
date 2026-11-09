#include "data.h"

static uint32_t count_a = 0U;
static uint32_t count_b = 0U;

void Data_Init(void)            { count_a = 0U; count_b = 0U; }
void Data_Reset(void)           { count_a = 0U; count_b = 0U; }
void Data_IncrementA(void)      { count_a++; }
void Data_IncrementB(void)      { count_b++; }
uint32_t Data_GetCountA(void)   { return count_a; }
uint32_t Data_GetCountB(void)   { return count_b; }
uint32_t Data_GetCountTotal(void) { return count_a + count_b; }