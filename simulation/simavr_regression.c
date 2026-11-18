/* simavr 회귀 테스트: 시나리오 A~I를 한 번에 실행 (SimulIDE와 같은 3바이트 복귀 주소 조건)
 * 빌드(리눅스): sudo apt install libsimavr-dev libelf-dev
 *   gcc -O1 -o simavr_regression simavr_regression.c -I/usr/include/simavr -lsimavr -lelf
 * 실행: ./simavr_regression ../firmware/main.elf
 * 2026-11-18 결과는 simavr_regression_result.txt 참조. */
#include <stdio.h>
#include "simavr/sim_avr.h"
#include "simavr/sim_elf.h"
#include "simavr/avr_ioport.h"
#include "simavr/avr_adc.h"
#include "simavr/avr_uart.h"
static int temp_mv=2219, gas_mv=500;  /* 20°C, 10% */
static void uart_cb(struct avr_irq_t*irq,uint32_t v,void*p){ putchar(v); fflush(stdout);}
static void adc_trig(struct avr_irq_t*irq,uint32_t v,void*p){ avr_t*avr=p; union{avr_adc_mux_t m; uint32_t v;} e={.v=v};
  avr_raise_irq(avr_io_getirq(avr,AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_ADC0+e.m.src), e.m.src==1?gas_mv:temp_mv);}
static avr_t*avr; static avr_irq_t*in;
static void pin(int bit,int v){ avr_ioport_external_t e={.name='D',.mask=1<<bit,.value=v?(1<<bit):0}; avr_ioctl(avr,AVR_IOCTL_IOPORT_SET_EXTERNAL('D'),&e);
  avr_raise_irq(avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('D'),bit),v);}
static void send(const char*s){ while(*s) avr_raise_irq(in,*s++); }
static const char*st(int a){ return a==1?"NORMAL(G)":a==2?"WARNING(Y)":a==4?"RED":a==0?"(off)":"?"; }
int main(int c,char**v){
  elf_firmware_t f={0}; elf_read_firmware(v[1],&f);
  avr=avr_make_mcu_by_name("atmega128"); avr_init(avr); avr->frequency=16000000; avr_load_firmware(avr,&f); avr->address_size=3;
  avr->aref=avr->avcc=5000;
  avr_irq_register_notify(avr_io_getirq(avr,AVR_IOCTL_UART_GETIRQ('0'),UART_IRQ_OUTPUT),uart_cb,0);
  avr_irq_register_notify(avr_io_getirq(avr,AVR_IOCTL_ADC_GETIRQ,ADC_IRQ_OUT_TRIGGER),adc_trig,avr);
  uint32_t fl=0; avr_ioctl(avr,AVR_IOCTL_UART_GET_FLAGS('0'),&fl); fl&=~AVR_UART_FLAG_STDIO; avr_ioctl(avr,AVR_IOCTL_UART_SET_FLAGS('0'),&fl);
  in=avr_io_getirq(avr,AVR_IOCTL_UART_GETIRQ('0'),UART_IRQ_INPUT);
  pin(0,1); pin(1,1);
  int la=-1,lo=-1,lb=-1; long bzt=0;
  for(int t=0;t<26000;t++){
    switch(t){
      case 200:  printf("\n== A: boot normal, S\n"); send("S\r"); break;
      case 300:  printf("\n== B: temp 45C(3.5V)\n"); temp_mv=3500; break;
      case 600:  temp_mv=2219; printf("\n== B end: temp back 20C\n"); break;
      case 800:  printf("\n== C: gas 3.0V\n"); gas_mv=3000; break;
      case 1500: printf("\n== C: gas back 0.5V (latched?) then R\n"); gas_mv=500; break;
      case 1600: send("R\r"); break;
      case 2000: printf("\n== D: flame press 50ms\n"); pin(0,0); break;
      case 2050: pin(0,1); break;
      case 2500: printf("   D check after release 450ms\n"); break;
      case 5200: printf("\n== D/E: (expect evacuate ~3s after D) G: reset button\n"); pin(1,0); break;
      case 5300: pin(1,1); break;
      case 6000: printf("\n== E: gas 3.0V hold >3s\n"); gas_mv=3000; break;
      case 9100: printf("\n== H: reset button 80ms after evacuate (pressed @9100)\n"); pin(1,0); gas_mv=500; break;
      case 9200: pin(1,1); break;
      case 10000:printf("\n== F: temp 3.5V + gas 3.0V together\n"); temp_mv=3500; gas_mv=3000; break;
      case 10300:temp_mv=2219; gas_mv=500; send("R\r"); break;
      case 11000:printf("\n== I: S T T (wait 3s) C S R O C x s\n"); send("S\r"); break;
      case 11100:send("T\r"); break;
      case 11200:send("T\r"); break;
      case 14500:send("C\r"); break;
      case 14600:send("S\r"); break;
      case 14700:send("R\r"); break;
      case 14800:send("O\r"); break;
      case 14900:send("C\r"); break;
      case 15000:send("x\r"); break;
      case 15100:send("s\r"); break;
    }
    uint64_t end=avr->cycle+16000; while(avr->cycle<end){int s=avr_run(avr); if(s==cpu_Crashed){printf("CRASH @%d\n",t);return 1;}}
    int a=avr->data[0x3B]&7, o=avr->data[0x4A]|(avr->data[0x4B]<<8), b=!!(avr->data[0x32]&0x10);
    if(a!=la && a!=0){printf("   [t=%dms] LED=%s\n",t+1,st(a)); la=a;}
    if(o!=lo){printf("   [t=%dms] servo=%s\n",t+1,o==3000?"OPEN":o==2000?"CLOSED":"?"); lo=o;}
    if(b!=lb){ if(b&&!lb) bzt++; lb=b;}
    if(t==15200) {printf("\nbuzzer rising edges total: %ld\nDONE\n",bzt); break;}
  }
}
