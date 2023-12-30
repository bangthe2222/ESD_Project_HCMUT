/*
 * Test.c
 *
 * Created: 18-Nov-23 1:20:19 PM
 * Author : Admin
 */ 
#define xtal 16000000UL				//F = 16MHz

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


unsigned char led_str[30];
unsigned char ch;
int i = 0; 
int hour=0, minute=0, second=0, time_div=0;
 uint8_t tData[3];
int Time_count = 0;
//uint8_t min = 5;					//Thoi gian cho de doc lai tin hieu trong che do Auto

uint8_t sensor = 0;						//Doc gia tri cam bien
uint8_t mode = 0;		//Khoi tao che do Auto = 0, Hen gio = 1

void UART_Init()
{
	//Set baud rate 9600
	UBRR0H = 0;
	UBRR0L = xtal/9600/16-1;
	UCSR0A = 0x00;
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);	//1 start bit - 1 stop bit - 8 bit frame
	UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);  // Enable rx, tx, interrupt khi nhan data
	sei();
}

//Gui data
void usart_putchar(char data)
{
	while ( !(UCSR0A & (1<<UDRE0)) ) ; //Kiem tra xem thanh ghi du lieu co rong hay khong?
	UDR0 = data;	// Ghi gia tri len thanh ghi du lieu
}

//Gui 1 string
void usart_write(unsigned char *string)
{
	int i = 0;
	for (i=0;i<255;i++)
	{
		if (string[i] !=0)
		{
			usart_putchar(string[i]);	
		}
		else break;
	}
}
//Doc trang thai cua den
int get_state(char mystring[30])
{	char state[3];							
	strncpy(state, mystring+0,3);			//Tach 3 ki tu dau tien khoi chuoi
	state[3] = '\0';
	if ((strcmp(state,"Mo")) ==0|(strcmp(state,"mo")) ==0) return 1;		//Neu la "Bat" hoac "bat" thi tra ve gia tri 1
	else if ((strcmp(state,"Tat")) == 0|(strcmp(state,"tat")) == 0) return 0;	//Neu la "Tat" hoac "Tat" thi tra ve gia tri 0
	else return 2;
}
/*------------------------------*/
//Doc che do cua den
int get_mode(char mystring[30])
{	char mode_str[7];
	strncpy(mode_str,mystring+7,7);			//Tach 3 ki tu dau tien khoi chuoi
	mode_str[7] = '\0';
	if ((strcmp(mode_str,"tu dong")) ==0) return 0;		//Auto mode = 0, Hen gio = 1
	else if ((strcmp(mode_str,"hen gio")) == 0) return 1;
	else return 2;	
}
/*------------------------------*/
//Doc gia tri thoi gian tu chuoi va chuyen tu string sang int
int get_time(char mystring[40])
{	char time_str[6];
	int i, n = 0, time_int ;
	for (i=0;i<strlen(mystring);i++)
	{ if ((mystring[i]>='0') && (mystring[i]<='9'))
		{	time_str[n] = mystring[i];
			n++;
		}
	}
	if (n == 0) return 0;
	else
	{	time_int = atoi(time_str);
		return time_int;
	}
}
/*----------------------------------*/

//-----------------------------------
//Dinh nghia cac duong giao tiep SPI tren AVR
#define  TWI_PORT	PORTC
#define  TWI_DDR	DDRC
#define	 TWI_PIN    PINC
#define	 SDA_PIN	4
#define  SCL_PIN    5

#define DS1307_SLA 0x68		//Dia chi mac dinh cua DS1307 la 0x68

//Dinh nghia gia tri cho thanh ghi toc do TWBR ung voi tan so xung nhip 8MHz

#define	 _100K 32
//----------------------------
//Bit R/W cho TWCR
#define TWI_W 0
#define TWI_R 1
//----------------------------
#define TWI_START	((1<<TWINT)|(1<<TWSTA)|(1<<TWEN))		//0xA4: Goi START Condition
#define TWI_STOP	((1<<TWINT)|(1<<TWSTO)|(1<<TWEN))		//0x94: Goi STOP Condition
#define TWI_CLR_TWINT   (1<<TWINT)|(1<<TWEN)					//0x84: xoa TWINT, NOT ACK
#define TWI_Read_ACK	(1<<TWINT)|(1<<TWEN)|(1<<TWEA)			//0xC4: xoa TWIN


//Khoi dong TWI
void TWI_init (void)
{
	TWSR = 0x00;				//Prescaler = 1
	TWBR = _100K;				//Toc do 100KHz
	TWCR = (1<<TWINT)|(1<<TWEN);
}
//------------------------------
//---------***Chuong trinh con giao tiep DS1307**------------------------
uint8_t TWI_DS1307_wadr(uint8_t	Addr)
{
	TWCR = TWI_START;					//Goi START Condition
	while ((TWCR & 0x80) == 0x00);		//Cho bit TWINT = 1, duong truyen ranh
	if ((TWSR & 0xF8) != 0x08) return TWSR;    //Neu goi START co loi thi thoat
	
	TWDR = (DS1307_SLA <<1) + TWI_W;	//Truyen dia chi va che do W vao SLAVE
	TWCR = TWI_CLR_TWINT;				//Xoa co TWINT va bat dau goi dia chi
	while ((TWCR & 0x80) == 0x00);		//Cho TWINT = 1
	if ((TWSR & 0xF8) != 0x18) return TWSR;   //Neu co loi khi truyen dia chi, thoat
	
	TWDR = Addr;						//Dua dia chi vao TWDR
	TWCR = TWI_CLR_TWINT;				//Clear bit TWINT va bat dau truyen dia chi	
	while((TWCR & 0x80) == 0x00);		//Cho co TWINT
	if ((TWSR & 0xF8) != 0x28) return TWSR;   //Neu truyen du lieu k thanh cong, thoat
	TWCR = TWI_STOP;					//Truyen STOP Condition
	return 0;
}
//------------------------------------
uint8_t TWI_DS1307_wblock(uint8_t Addr, uint8_t Data[], uint8_t len)  // Ghi 1 mang vao DS
{
	TWCR = TWI_START;						//START Condition
	while ((TWCR & 0x80)==0x00);			//Cho co TWINT = 1
	if ((TWSR & 0xF8)!=0x08) return TWSR;	//Neu co loi thi thoat
	
	TWDR = (DS1307_SLA<<1)+TWI_W;			//Truyen dia chi DS1307 va bit W
	TWCR = TWI_CLR_TWINT;					//Xoa co TWINT
	while ((TWCR & 0x80)==0x00);			//Cho co TWINT = 1
	if ((TWSR & 0xF8)!=0x18) return TWSR;	//Neu co loi thi thoat
	
	TWDR = Addr;							//Dua dia chi thanh ghi can ghi vao
	TWCR = TWI_CLR_TWINT;					//Xoa co TWINT va truyen data
	while ((TWCR & 0x80)==0x00);			//Cho co TWINT = 1
	if ((TWSR & 0xF8)!=0x28) return TWSR;	//Neu co loi thi thoat
	
	for (uint8_t j =0; j<len; j++)
	{
		TWDR = Data[j];						//Chuan bi du lieu
		TWCR = TWI_CLR_TWINT;				//Xoa TWINT, bat dau send
		while ((TWCR & 0x80)==0x00);			//Cho co TWINT = 1
		if ((TWSR & 0xF8)!=0x28) return TWSR;	//Neu co loi thi thoat
	}
	TWCR = TWI_STOP;
	return 0;
}

uint8_t TWI_DS1307_rblock(uint8_t Data[], uint8_t len)	//Doc 1 mang tu DS1307
{
	uint8_t j;
	TWCR = TWI_START;									//START Condition
	while (((TWCR & 0x80)==0x00)||((TWSR & 0xF8)) != 0x08); //Cho TWINT=1 hoac 0x08
	
	TWDR = (DS1307_SLA<<1)+TWI_R;						//Truyen dia chi + R
	TWCR = TWI_CLR_TWINT;
	while (((TWCR & 0x80)==0x00)||((TWSR & 0xF8)!=0x40)); //CHo TWIN = 1 hoac 0x40
	//nhan len-1 bytes dau tien----------------
	for (j=0; j<len-1; j++)
	{
		TWCR = TWI_Read_ACK;							//Xoa TWINT va gui ACK
		while (((TWCR & 0x80)==0x00)||((TWSR & 0xF8)!=0x50)); //CHo TWIN = 1 hoac 0x50
		Data[j] = TWDR;									//Ghi du lieu vao Data
	}
	//Nhan byte cuoi
	TWCR = TWI_CLR_TWINT;								//Xoa TWINT de nhan byte cuoi
	while (((TWCR & 0x80)==0x00)||((TWSR & 0xF8)!=0x58)); //Cho TWIN = 1 hoac 0x58 va gui NOT ACK
	Data[len-1] = TWDR;
	TWCR=TWI_STOP;										//STOP Condition
	return 0;
}
//Chuyen so BCD sang DEC
uint8_t BCD_to_DEC (uint8_t BCD)
{
	uint8_t L, H;
	L = BCD & 0x0F;					//Doc chu so hang dvi
	H = (BCD>>4)*10;				//Doc chu so hang chuc
	return (H+L);
}
uint8_t DEC_to_BCD (uint8_t dec)
{
	uint8_t L, H;
	L = dec%10;					//Ghi chu so hang dvi
	H = (dec/10)<<4;			//Ghi chu so hang chuc
	return (H+L);
}
//-------------------------------------------------------------------------------------------------*/

//Dua ky tu nhan duoc vao chuoi
void receive_str(unsigned char data)
{
	if (data !='\n')				//Kiem tra co phai ky tu ket thuc hong?
	{
		led_str[i] = data;		//Dua ki tu vao chuoi
		i++;
	}
	else
	  {	i = 0;

		if ( (get_mode(led_str)==1) )
			{ mode = 1;
				PORTB = 0x00|(1<<PORTB3);
				PORTD = (1<<PORTD2);
			 clr_str(led_str);
			}
		else if ( (get_mode(led_str)==0) )
			{ mode = 0;
				PORTB = 0x00|(1<<PORTB2) ;
				PORTD = (1<<PORTD2);
				clr_str(led_str);
			}
		else if ( (get_state(led_str)) && (mode==1) )
			{	
				int time_tmp=0;
				PORTB = 0x00|(1<<PORTB2)|(1<<PORTB4);		//Bat den
			//-----------------------------------------------------------
			//Tinh toan thoi gian cai dat
				time_tmp = get_time(led_str);
				second = time_tmp%60;			//Doi sang giay
				time_div = time_tmp/60;
				hour = time_div/60;			//Doi sang gio

			//-----------------------------------------------------------
			//Truyen du lieu thoi gian tu AVR sang RTC
			tData[0] = 0x00;				//Chuyen sec sang BCD
			tData[1] = 0x00;				//Chuyen min sang BCD
			tData[2] = 0x00;				//Chuyen gio sang BCD
			TWI_DS1307_wblock(0x00,tData,3);		//Ghi lien tiep cac bien thoi gian vao RTC
			_delay_ms(1);					//Cho DS1307 xu ly
			//-------------------------------------------------
			//Doc lan dau gia tri thoi gian
			TWI_DS1307_wadr(0x00);			//Set dia chi ve 0x00
			_delay_ms(1);					//Cho DS1307 xu ly
			TWI_DS1307_rblock(tData,3);		//Doc thoi gian tu DS1307
			//--------------------------------------------------
			clr_str(led_str);
			} 
		else if ( !(get_state(led_str)) && (mode==1) ) 
			{	PORTB = 0x00|(1<<PORTB2);				//Tat den
				clr_str(led_str);					//Xoa led_str
			}
		
	}
}
//-------------------------------------------------------
//********************Xoa mang led_str*******************
void clr_str (char *string)
{
	for (int i = 0; i<strlen(string);i++)
	{
		string[i] = 0x00;
	}
}
//CTC ngat Timer0
ISR (TIMER1_OVF_vect)
{
	TCNT1 = 40536;
	int sec_temp;
	int min_temp;
	int hour_temp;

	TWI_DS1307_wadr(0x00);			//Set dia chi ve 0x00
	_delay_ms(1);					//Cho DS1307 xu ly
	TWI_DS1307_rblock(tData,3);		//Doc thoi gian tu DS1307
	sec_temp = BCD_to_DEC(tData[0]);
	min_temp = BCD_to_DEC(tData[1]);
	hour_temp = BCD_to_DEC(tData[2]);
	if ( (sec_temp==second) && (min_temp==minute) && (hour_temp==hour) && ((sec_temp!=0)|(min_temp!=0)|(hour_temp!=0)) ) //Kiem tra xem du thoi gian chua?
			{  PORTB = 0x00|(1<<PORTB3);
				second = 0;
				minute = 0;
				hour = 0;
			}


		 
}
//Nhan 1 string

ISR(USART_RX_vect) 
{
	ch = UDR0;
	receive_str(ch);
	}
//--------------------------
//Chuong trinh phuc vu ngat INT0
ISR (INT0_vect)
{
	 sensor = PIND & 0x04;			//Doc sensor
	 if ( (sensor == 0x04 ) && (mode==0))	PORTB = 0x00|(1<<PORTB4)|(1<<PORTB2); //Bat den chieu sang, hien thi den bao mode 
	 else if ( (sensor == 0x00 ) && (mode==0))
		 PORTB = 0x00|(1<<PORTB2);						//Tat den chieu sang, hien thi den bao mode 

}

int main(void)
{	UART_Init();
	DDRB = 0x00|(1<<PORTB2)|(1<<PORTB3)|(1<<PORTB4);			//PB2, PB3, PB4 la output
	TWI_init();				//Khoi dong TWI
	
	DDRD = 0x00;
	
	//Init Timer0
	TCCR1B |= (1<<CS11)|(1<<CS10);			//Prescaler = 64
	TIMSK1 = (1<<TOIE1);					//Cho phep ngat khi Timer0 OVF
	TCNT1 = 40536;
	//Init Interrupt
	EIMSK = (1<<INT0);						//Cho phep ngat ngoai INT0
	EICRA = (1<<ISC00);						//Ngat khi co bat ky su thay doi nao cua PD2

    /* Replace with your application code */
    while (1) 
    { 	
		   
    }

}

