/*

set param

плата: esp8266 Boards -> Generic esp8266 module
все інше по дефолту

Програматор: usb->uart for esp-01

*/

#include <ESP8266WiFi.h>
#include <stdio.h>
#include <EEPROM.h>

#define EEPROM_SIZE 100
#define SN_ESP_ADR 0

#include "conf_my.h"
//#include "conf.h"

#define HEX_REQUEST
#define IO2_DSBL

long sn_esp;
byte inic_conn;
byte perev_adminpass;

byte byte_requbuf[600];

WiFiClient client;

byte inic_sn_mega;

byte send_fl;
byte resv_buf[18];
byte send_buf[33];

void clearSerialBuffer();

unsigned long startTime;  // Запам'ятовуємо час початку очікування
unsigned long timeout;  // Таймаут у мілісекундах (наприклад, 5 секунд)

short adminpass;


class MyRand{
	unsigned long q, w, e, r, t;
	byte y;
	void nullRand() {
		q = KEY_1;
		w = KEY_2;
		e = KEY_3;
		r = KEY_4;
	}
	public:
		MyRand() {
			nullRand();
		}
		byte rand() {
			t = q ^ (q << 11);
			q = w;
			w = e;
			e = r;
			r = (r ^ (r >> 19)) ^ (t ^ (t >> 8));
			y = (byte)(r ^ (r >> 8) ^ (r >> 16) ^ (r >> 24));
			return y;
		}
		void srand(byte* zerno) {
			nullRand();
			for(byte ii = 0; ii < 10; ii += 2) {
				q ^= ((unsigned long*)zerno)[ii];
				e ^= ((unsigned long*)zerno)[ii + 1];
			}
			rand();
			rand();
			rand();
			rand();
		}
		void korr(byte ks) {
			q ^= (unsigned long)ks; 
			rand();
			rand();
			rand();
			rand();
		}
} my_rand;


bool send_mes() {
sss:
	ESP.wdtFeed();
	clearSerialBuffer();
	resv_buf[17] = (byte)0b10101010;
	for(byte ii = 0; ii < 17; ii++)
		resv_buf[17] ^= resv_buf[ii];
	Serial.write(resv_buf, 18);
	if((resv_buf[16] & 0xf0) != 0xf0 && (resv_buf[16] & 0xf0) != 0xe0) {
		char i = 0;
		while(Serial.available() < 33) {
			delay(1);
			i++;
			if(i > 120)
				goto sss;
		}
		Serial.readBytes(send_buf, 33);
		byte prov = (byte)0b10101010;
		for(byte ii = 0; ii < 32; ii++)
			prov ^= send_buf[ii];
		if(prov != send_buf[32])
			goto sss;
	}
	else {
		char i = 0;
		while(Serial.available() < 3) {
			delay(1);
			i++;
			if(i > 120)
				goto sss;
		}
		Serial.readBytes(send_buf, 3);
		if(send_buf[0] == 'W' && send_buf[1] == 'F' && send_buf[2] == 0xff) {
			ESP.restart();
			return 1;
		}
		if(send_buf[0] != 'O' || send_buf[1] != 'K')
			goto sss;
	}
	return 1;
}


byte dsplbuf[128];
byte obscht_r[4], tek_r[4];
byte in[2], in_mem[2];
byte cory;
byte navihacija_menu;
byte rezhym_tmr;
byte rz[3][13];
byte ri[3][17];
byte rv[3][19];
byte arr_golovne_menu[16];

char requbuf[1500];

bool getWiFiSettings();
bool getUpdt();
bool getAll();

char ssid[33];
char password[33];
char host[33];
int port;
byte local_IP_B[4];
byte subnet_B[4];
byte gateway_B[4];
byte primaryDNS_B[4];
byte secondaryDNS_B[4];
byte dhcp_fl;
long sn_mega;
char name[33];


bool getDispl() {
	resv_buf[16] = 0x01;
	send_mes();
	for(byte ii = 0; ii < 32; ii++)
		dsplbuf[ii] = send_buf[ii];
	resv_buf[16] = 0x02;
	send_mes();
	for(byte ii = 0; ii < 32; ii++)
		dsplbuf[ii + 32] = send_buf[ii];
	resv_buf[16] = 0x03;
	send_mes();
	for(byte ii = 0; ii < 32; ii++)
		dsplbuf[ii + 64] = send_buf[ii];
	resv_buf[16] = 0x04;
	send_mes();
	for(byte ii = 0; ii < 32; ii++)
		dsplbuf[ii + 96] = send_buf[ii];
	return 1;
}


bool getAct() {
	if(inic_sn_mega == 1) {
		resv_buf[16] = 0xec;
		*((long*)(&(resv_buf[0]))) = sn_mega;
		send_mes();
		inic_sn_mega = 0;
	}
	byte k = 0;
	do{
		resv_buf[16] = 0x05;
		send_mes();
		for(byte i = 0; i < 30; i++)
			k |= send_buf[i];
	}while(k == 0);
	for(byte ii = 0; ii < 4; ii++) {
		obscht_r[ii] = send_buf[ii];
		tek_r[ii] = send_buf[4 + ii];
	}
	in[0] = send_buf[8];
	in[1] = send_buf[9];
	in_mem[0] = send_buf[10];
	in_mem[1] = send_buf[11];
	cory = send_buf[12];
	navihacija_menu = send_buf[13];
	rezhym_tmr = send_buf[14];
	for(byte ii = 0; ii < 3; ii++) {
		rv[ii][14] = send_buf[ii * 2 + 15];
		rv[ii][15] = send_buf[ii * 2 + 16];
		ri[ii][14] = send_buf[ii * 2 + 21];
		ri[ii][15] = send_buf[ii * 2 + 22];
	}
	ri[2][16] = send_buf[27] & 0x01;
	rv[2][18] = send_buf[28] & 0x01;
	send_buf[27] >>= 1;
	send_buf[28] >>= 1;
	ri[1][16] = send_buf[27] & 0x01;
	rv[1][18] = send_buf[28] & 0x01;
	send_buf[27] >>= 1;
	send_buf[28] >>= 1;
	ri[0][16] = send_buf[27] & 0x01;
	rv[0][18] = send_buf[28] & 0x01;
	send_buf[27] >>= 1;
	send_buf[28] >>= 1;
	rz[2][12] = send_buf[27] & 0x01;
	rv[2][16] = send_buf[28] & 0x01;
	send_buf[27] >>= 1;
	send_buf[28] >>= 1;
	rz[1][12] = send_buf[27] & 0x01;
	rv[1][16] = send_buf[28] & 0x01;
	send_buf[27] >>= 1;
	send_buf[28] >>= 1;
	rz[0][12] = send_buf[27] & 0x01;
	rv[0][16] = send_buf[28] & 0x01;
	send_buf[27] >>= 1;
	send_buf[28] >>= 1;
	rv[1][17] = send_buf[27] & 0x01;
	rv[2][17] = send_buf[28] & 0x01;
	send_buf[27] >>= 1;
	rv[0][17] = send_buf[27] & 0x01;
	
	adminpass = *((short*)(&(send_buf[29])));
	*((short*)(&(send_buf[29]))) = 0;
	
	if(send_buf[31] & 0b11111111) {
		ESP.restart();
	}
	if(send_buf[30] & 0b00000001) {
		getAll();
		send_buf[30] &= 0b11111100;
	}
	if(send_buf[30] & 0b00000010) {
		getUpdt();
	}
	
	return 1;
}


/*
0x31 ssid
0x32 pass
0x33 host
0x34 oter
*/
bool getWiFiSettings() {
	resv_buf[16] = 0x31;
	byte ii;
	if(send_mes()) {
		for(ii = 0; ii < 32 && send_buf[ii] != 0; ii++) {
			ssid[ii] = (char)send_buf[ii];
		}
		ssid[ii] = 0;
		ssid[32] = 0;
	}
	else 
		return 0;
	
	resv_buf[16] = 0x32;
	if(send_mes()) {
		for(ii = 0; ii < 32 && send_buf[ii] != 0; ii++) {
			password[ii] = (char)send_buf[ii];
		}
		password[ii] = 0;
		password[32] = 0;
	}
	else 
		return 0;
	
	resv_buf[16] = 0x33;
	if(send_mes()) {
		for(ii = 0; ii < 32 && send_buf[ii] != 0; ii++) {
			host[ii] = (char)send_buf[ii];
		}
		host[ii] = 0;
		host[32] = 0;
	}
	else 
		return 0;
	
	resv_buf[16] = 0x34;
	if(send_mes()) {
		for(byte ii = 0; ii < 4; ii++) {
			local_IP_B[ii] = send_buf[ii];
			subnet_B[ii] = send_buf[ii + 4];
			gateway_B[ii] = send_buf[ii + 8];
			primaryDNS_B[ii] = send_buf[ii + 12];
			secondaryDNS_B[ii] = send_buf[ii + 16];
		}
		port = (int)(*((long*)(&(send_buf[20]))));
		dhcp_fl = send_buf[24];
		sn_mega = *((long*)(&(send_buf[25])));
//		sn_esp = SN;
	}
	else 
		return 0;
	
	resv_buf[16] = 0x35;
	if(send_mes()) {
		for(ii = 0; ii < 32 && send_buf[ii] != 0; ii++) {
			name[ii] = (char)send_buf[ii];
		}
		name[ii] = 0;
		name[32] = 0;
	}
	else 
		return 0;
	
	return 1;
}


bool getUpdt() {
	resv_buf[16] = 0x14;
	if(send_mes()) {
		for(byte ii = 0; ii < 16; ii++) {
			arr_golovne_menu[ii] = send_buf[ii];
		}
		for(byte ii = 0; ii < 3; ii++) {
			rv[ii][12] = send_buf[ii * 2 + 16];
			rv[ii][13] = send_buf[ii * 2 + 17];
			ri[ii][12] = send_buf[ii * 2 + 22];
			ri[ii][13] = send_buf[ii * 2 + 23];
		}
	}
	else 
		return 0;
	
	return 1;
}


bool getAll() {
	resv_buf[16] = 0x10;
	if(send_mes()) {
		for(byte ii = 0; ii < 12; ii++) {
			rz[0][ii] = send_buf[ii];
			rz[1][ii] = send_buf[12 + ii];
			if(ii < 8)
				rz[2][ii] = send_buf[24 + ii];
		}
	}
	else 
		return 0;
	resv_buf[16] = 0x11;
	if(send_mes()) {
		for(byte ii = 0; ii < 14; ii++) {
			if(ii >= 8 && ii < 12)
				rz[2][ii] = send_buf[ii - 6];
			ri[0][ii] = send_buf[ii + 4];
			ri[1][ii] = send_buf[ii + 18];
		}
	}
	else 
		return 0;
	resv_buf[16] = 0x12;
	if(send_mes()) {
		for(byte ii = 0; ii < 14; ii++) {
			ri[2][ii] = send_buf[ii];
			rv[0][ii] = send_buf[ii + 14];
		}
	}
	else 
		return 0;
	resv_buf[16] = 0x13;
	if(send_mes()) {
		for(byte ii = 0; ii < 14; ii++) {
			rv[1][ii] = send_buf[ii];
			rv[2][ii] = send_buf[ii + 14];
		}
	}
	else 
		return 0;
	
	return getUpdt();
}

/*
bool  () {
	char test[] = "test ' ' test  ";
	static char a = 'A';
	for(byte ii = 0; ii < 14; ii++)
		resv_buf[ii] = (byte)test[ii];
	resv_buf[14] = 0;
	resv_buf[16] = 0x20;
	resv_buf[6] = a++;
	if(a > 'Z')
		a = 'A';
	send_mes();
	return 1;
}
*/

/*
dija_vk_return:
rele all 			0xFX 
time				0xFA
save_adminset		0xFB
		bez ochykuvannja vidpovidi
		(abo minimal`na, nakshtalt odnogo bajta 'OK')
set b0-b5			0xEX		(z vtorogo short'a)
set interface		0xEA
set rezhym_tmr		0xEB
set adminpass		0xE0
*/
//bool setAdmin


void read_str_eepr(int adr, int length, char* str) {
	int i;
	for(i = 0; i < length && EEPROM.read(i + adr) != 0; i++) {
		*(&(str[0]) + i) = (char)EEPROM.read(i + adr);
	}
	*(&(str[0]) + i) = 0;
}


void write_str_eepr(int adr, int length, char* str) {
	int i;
	for(i = 0; i < length && *(&(str[0]) + i) != 0; i++) {
		EEPROM.write(i + adr, (char)(*(&(str[0]) + i)));
	}
	EEPROM.write(i + adr, (char)0);
	EEPROM.commit();
}


void setup() {
	Serial.begin(9600);
	ESP.wdtDisable();
#ifdef IO2_DSBL
	pinMode(2, INPUT);
	digitalWrite(2, 1);
#endif
	inic_sn_mega = 0;
	perev_adminpass = 0;
	EEPROM.begin(EEPROM_SIZE);
	read_str_eepr(SN_ESP_ADR, 4, (char*)(&sn_esp));
	delay(500);
	resv_buf[16] = 0xff;
	send_mes();
	delay(500);
	inic_conn = 1;
	while(!getWiFiSettings())
		delay(5000);
	IPAddress local_IP(local_IP_B[0], local_IP_B[1], local_IP_B[2], local_IP_B[3]);
	IPAddress subnet(subnet_B[0], subnet_B[1], subnet_B[2], subnet_B[3]);
	IPAddress gateway(gateway_B[0], gateway_B[1], gateway_B[2], gateway_B[3]);
	IPAddress primaryDNS(primaryDNS_B[0], primaryDNS_B[1], primaryDNS_B[2], primaryDNS_B[3]);
	IPAddress secondaryDNS(secondaryDNS_B[0], secondaryDNS_B[1], secondaryDNS_B[2], secondaryDNS_B[3]);
	WiFi.mode(WIFI_STA);
	if(!dhcp_fl)
		WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
	WiFi.begin(ssid, password);
	resv_buf[16] = 0xff;
	send_mes();
	while (WiFi.status() != WL_CONNECTED) {
		resv_buf[16] = 0xff;
		send_mes();
		delay(500);
	}
	delay(500);
	ESP.wdtEnable(WDTO_8S);
	getAll();
	getDispl();
}


void clearSerialBuffer() {
	while (Serial.available() > 0) {
		char junk = Serial.read();
	}
}


char HiHex(byte bb) {
	return LoHex(bb >> 4);
}


char LoHex(byte bb) {
	bb &= 0x0f;
	if(bb < 10) {
		return (char)(bb + 48);
	}
	else {
		return (char)((byte)'A' + (bb - 10));
	}
}


short formBuf() {
	short ii, jj = 0;
	for(ii = 0; ii < 320; ii++)
		byte_requbuf[ii] = 0;
	
	if(!inic_conn) {
		for(ii = 0; ii < 128; ii++) {
			byte_requbuf[jj] = dsplbuf[ii];
			jj++;
		}
		for(ii = 0; ii < 4; ii++) {
			byte_requbuf[jj] = obscht_r[ii];
			jj++;
		}
		for(ii = 0; ii < 4; ii++) {
			byte_requbuf[jj] = tek_r[ii];
			jj++;
		}
		for(ii = 0; ii < 2; ii++) {
			byte_requbuf[jj] = in[ii];
			jj++;
		}
		for(ii = 0; ii < 2; ii++) {
			byte_requbuf[jj] = in_mem[ii];
			jj++;
		}
		byte_requbuf[jj] = cory;
		jj++;
		byte_requbuf[jj] = navihacija_menu;
		jj++;
		byte_requbuf[jj] = rezhym_tmr;
		jj++;
		for(ii = 0; ii < 39; ii++) {
			byte_requbuf[jj] = *(&(rz[0][0]) + ii);
			jj++;
		}
		for(ii = 0; ii < 51; ii++) {
			byte_requbuf[jj] = *(&(ri[0][0]) + ii);
			jj++;
		}
		for(ii = 0; ii < 57; ii++) {
			byte_requbuf[jj] = *(&(rv[0][0]) + ii);
			jj++;
		}
		for(ii = 0; ii < 16; ii++) {
			byte_requbuf[jj] = arr_golovne_menu[ii];
			jj++;
		}
		byte_requbuf[jj] = (byte)0x1a;//ЯКАСЬ галімотья
		jj++;
		byte_requbuf[jj] = (byte)0xa1;
		jj++;
		byte_requbuf[jj] = (byte)0x1a;
		jj++;
		byte_requbuf[jj] = (byte)0xa1;
		jj++;
		if(perev_adminpass) {
			byte_requbuf[jj] = perev_adminpass;
			perev_adminpass = 0;
		}
		*((long*)(&(byte_requbuf[312]))) = sn_esp;
		*((long*)(&(byte_requbuf[316]))) = sn_mega;
	}
	else {
		for(ii = 0; ii < 28; ii++) {
			byte_requbuf[jj] = name[ii];
			jj++;
		}
		my_rand.korr((byte)(micros() & 0xff));
		byte_requbuf[28] = my_rand.rand();
		byte_requbuf[29] = my_rand.rand();
		byte_requbuf[30] = my_rand.rand();
		byte_requbuf[31] = my_rand.rand();
		if(!sn_esp || sn_esp == 0xffffffff) {
			((byte*)(&sn_esp))[0] = my_rand.rand();
			((byte*)(&sn_esp))[1] = my_rand.rand();
			((byte*)(&sn_esp))[2] = my_rand.rand();
			((byte*)(&sn_esp))[3] = my_rand.rand();
			write_str_eepr(SN_ESP_ADR, 4, (char*)(&sn_esp));
		}
		jj += 4;
		*((long*)(&(byte_requbuf[jj]))) = sn_esp;
		jj += 4;
		if(!sn_mega || sn_mega == 0xffffffff) {
			((byte*)(&sn_mega))[0] = my_rand.rand();
			((byte*)(&sn_mega))[1] = my_rand.rand();
			((byte*)(&sn_mega))[2] = my_rand.rand();
			((byte*)(&sn_mega))[3] = my_rand.rand();
			inic_sn_mega = 1;
		}
		*((long*)(&(byte_requbuf[jj]))) = sn_mega;
		jj += 4;
	}
	return jj;
}


void codBuf(short jj) {
	short ii = 0;
	byte_requbuf[311] = 0;
	for(; ii < 311; ii++) {
		byte_requbuf[311] ^= byte_requbuf[ii];
		byte_requbuf[ii] ^= my_rand.rand();
	}
	my_rand.korr(byte_requbuf[311]);
}


bool firewall(byte command) {
	if((command >= (byte)0xe1 && command <= (byte)0xea) || (command >= (byte)0xf1 && command <= (byte)0xfa))
		return true;
	return false;
}


byte decodBuf(short jj) {
//	if(requbuf[9] != '2' || requbuf[10] != '0' || requbuf[11] != '0')
//		return 0;
	
//	resv_buf[16] = 0xfe;
//	send_mes();
	byte ks = 0;
	short ii = 0;
	if(jj == 0) {
		for(; requbuf[ii + 3] != 0; ii++) {
			if(requbuf[ii] == 13 && requbuf[ii + 1] == 10 && requbuf[ii + 2] == 13 && requbuf[ii + 3] == 10)
				break;
		}
		ii += 4;
		if(requbuf[ii] == 0)
			return 0;
		for(jj = 0; requbuf[ii] != 0 && requbuf[ii + 1] != 0;) {
			byte_requbuf[jj] = (requbuf[ii] <= '9' ? (requbuf[ii] - '0') : (requbuf[ii] - 'A' + 10));
			byte_requbuf[jj] <<= 4;
			byte_requbuf[jj] += (requbuf[ii + 1] <= '9' ? (requbuf[ii + 1] - '0') : (requbuf[ii + 1] - 'A' + 10));
			jj++;
			ii += 2;
			if(requbuf[ii] == 0)
				break;
			ii++;
		}
		jj--;
//		sprintf(&(name[2]), "   %02x ", jj);
	}
	else {
		String dbg_msg = "";
		char bbb[33];
		
		
		for(; requbuf[ii + 3] != 0; ii++) {
			if(!strncmp((char*)(&(requbuf[ii])), "Content-Length:", 10)) {
				jj = atoi(&(requbuf[ii + 15]));
			}
			if(requbuf[ii] == 13 && requbuf[ii + 1] == 10 && requbuf[ii + 2] == 13 && requbuf[ii + 3] == 10)
				break;
		}
		ii += 4;
		
		sprintf(bbb, "Content-Length: %d", jj);
		dbg_msg += bbb;
		dbg_msg += "\r\n";
//		requbuf[ii + jj] = 0;
//		requbuf[ii + jj + 1] = 0;
//		requbuf[ii + jj + 2] = 0;
		dbg_msg += requbuf;
		send_mes_debag(dbg_msg);
//		sprintf(&(name[5]), "%d %d", jj, ii);
		for(short kk = 0; kk < jj; kk++) {
			requbuf[kk] = requbuf[ii + kk];
			if(requbuf[kk] == 0x2c)
				requbuf[kk] = 0x5c;
		}
		
		short blocks;
		byte reshta = requbuf[jj - 1] & 0x03;
		if(reshta == 0) {
			blocks = (jj - 1) / 4;
			jj = blocks * 3;
		}
		else {
			blocks = (jj - 1) / 4;
			jj = (blocks - 1) * 3 + reshta;
		}
		
		for(ii = 0; ii < blocks; ii++) {
			byte_requbuf[ii * 3 + 0] = ((requbuf[ii * 4 + 0] & 0x3f) << 2) | ((requbuf[ii * 4 + 1] & 0x30) >> 4);
			byte_requbuf[ii * 3 + 1] = ((requbuf[ii * 4 + 1] & 0x0f) << 4) | ((requbuf[ii * 4 + 2] & 0x3c) >> 2);
			byte_requbuf[ii * 3 + 2] = ((requbuf[ii * 4 + 2] & 0x03) << 6) | ((requbuf[ii * 4 + 3] & 0x3f) >> 0);
		}
//		sprintf(&(name[0]), "%02X %02X %02X %02X %02X %02X  %d", byte_requbuf[0], byte_requbuf[1], byte_requbuf[2], byte_requbuf[3], byte_requbuf[4], byte_requbuf[5], jj);
		jj--;
	}
	ks = 0;
	for(ii = 0; ii < jj; ii++) {
		byte_requbuf[ii] ^= my_rand.rand();
		ks ^= byte_requbuf[ii];
	}

//	sprintf(&(name[20]), "%02x %02x", ks, byte_requbuf[jj]);
	if(ks != byte_requbuf[jj]) {
		send_mes_debag("ks Error");
		return 0;
	}
	my_rand.korr(ks);
//	sprintf(&(name[5]), " %.10s", (char*)byte_requbuf);
	byte_requbuf[jj] = 0;
	char bbb[33];
	sprintf(bbb, "%d", jj);
	String dbg_msg = "";
	dbg_msg += "jj = ";
	dbg_msg += bbb;
//	dbg_msg += "\r\n";
//	dbg_msg += (char*)byte_requbuf;
	send_mes_debag(dbg_msg);
	if(inic_conn == 1) {
		if(strcmp((char*)byte_requbuf, "ok_chuvak_ja_tebe_pojnjav_davaj_korysni_dani"))
			return 0;
	}
	else {
		if(strcmp((char*)byte_requbuf, "ok_chuvak_ja_tebe_pojnjav_davaj_korysni_dani")) {
			short xx = 0;
			xx += byte_requbuf[jj - 3];
			xx <<= 8;
			xx += byte_requbuf[jj - 4];
			if((jj % 17) == 4) {
				short kk = jj / 17;
				if(adminpass == xx && byte_requbuf[jj - 2] == (byte)0xa5 && byte_requbuf[jj - 1] == (byte)0xa5) {
					for(ii = 0; ii < kk; ii++) {
						if(firewall(byte_requbuf[16 + (ii * 17)])) {
							for(short mm = 0; mm < 17; mm++) {
								resv_buf[mm] = byte_requbuf[mm + (ii * 17)];
							}
							send_mes();
		//					sprintf(&(name[0]), " jj=%d kk=%d ii=%d []=%d", jj, kk, ii, kk * 17 + 2 + 1);
		//					sprintf(&(name[0]), " [0]=%02X [1]=%02X [2]=%02X [3]=%02X ", byte_requbuf[kk * 17 + 0], byte_requbuf[kk * 17 + 1], byte_requbuf[kk * 17 + 2], byte_requbuf[kk * 17 + 3]);
						}
						else {
							perev_adminpass = 2;
							return 1;
						}
					}
					perev_adminpass = 1;
					return 1;
				}
				perev_adminpass = 2;
			}
			else {
				inic_conn = 1;
				return 0;
			}
		}
	}
	return 1;
}


short formRequbuf(short length) {
	short ii, jj = 0;
	for(ii = 0; ii < 999; ii++)
		requbuf[ii] = 0;
	for(ii = 0; ii < length; ii++) {
		requbuf[jj] = HiHex(byte_requbuf[ii]);
		requbuf[jj + 1] = LoHex(byte_requbuf[ii]);
		requbuf[jj + 2] = '+';
		jj += 3;
	}
	jj--;
	requbuf[jj] = 0;
	return jj;
}


short formRequbuf_bytes(short length) {
	byte reshta = length % 3;
	short blocks = length / 3 + (reshta == 0 ? 0 : 1);
	short length2 = blocks * 4 + 1;
	for(short ii = 0; ii < blocks; ii++) {
		requbuf[ii * 4 + 0] = 0x40 | ((byte_requbuf[ii * 3 + 0] & 0b11111100) >> 2);
		requbuf[ii * 4 + 1] = 0x40 | ((byte_requbuf[ii * 3 + 0] & 0b00000011) << 4) | ((byte_requbuf[ii * 3 + 1] & 0b11110000) >> 4);
		requbuf[ii * 4 + 2] = 0x40 | ((byte_requbuf[ii * 3 + 1] & 0b00001111) << 2) | ((byte_requbuf[ii * 3 + 2] & 0b11000000) >> 6);
		requbuf[ii * 4 + 3] = 0x40 | ((byte_requbuf[ii * 3 + 2] & 0b00111111) >> 0);
	}
	for(short ii = 0; ii < length2; ii++) {
		if(requbuf[ii] == 0x5c)
			requbuf[ii] = 0x2c;
	}
	requbuf[length2 - 1] = 0x20 | reshta;
	requbuf[length2] = 0;
	return length2;
}


void send_mes_debag(String send) {
	
	return;
	
	char bbb[1500];
	
	if (client.connect(host, port)) {
		client.print("POST ");
		client.print(DBG_PATH);
		client.print(" HTTP/1.1\r\n");
		client.print("Host: ");
		client.print(host);
		client.print("\r\n");
		client.print("Content-Type: application/octet-stream\r\n");
		client.print("User-Agent: ESP8266\r\n");
		client.printf("Content-Length: %d\r\n", send.length());
		client.print("Connection: close\r\n\r\n");
		client.print(send);
		while(client.connected()) {
			if(client.available()) {
				bbb[client.read((uint8_t*)bbb, 1500)] = 0;
			}
		}
		client.stop();
	}
}


void loop() {
//	char requbuf2debag[999];
	while(true) {
//		ESP.wdtFeed();
		delay(5000);
//		ESP.wdtFeed();
		getAct();
		getDispl();
		if (client.connect(host, port)) {
			if(inic_conn)
				client.print("POST ");
			else
				client.print("PUT ");
			client.print(LOOP_PATH);
			client.print(" HTTP/1.1\r\n");
			client.print("Host: ");
			client.print(host);
			client.print("\r\n");
			client.print("User-Agent: ESP8266\r\n");
#ifdef HEX_REQUEST
			client.print("Content-Type: application/octet-stream\r\n");
#endif
//			client.print("Accept-Language: uk-ua\r\n");
			if(inic_conn) {
				formBuf();
				my_rand.srand(byte_requbuf);
#ifndef HEX_REQUEST
				client.printf("Content-Length: %d\r\n", formRequbuf(40));
#else
				client.printf("Content-Length: %d\r\n", formRequbuf_bytes(40));
#endif
			}
			else {
				formBuf();
				codBuf(320);
#ifndef HEX_REQUEST
				client.printf("Content-Length: %d\r\n", formRequbuf(320));
#else
				client.printf("Content-Length: %d\r\n", formRequbuf_bytes(320));
#endif
			}
			client.print("Connection: close\r\n\r\n");
			client.print(requbuf);
			while(client.connected()) {
				delay(1);
				if(client.available()) {
					requbuf[client.read((uint8_t*)requbuf, 1500)] = 0;
				}
			}
#ifndef HEX_REQUEST
			if(decodBuf(0)) {
#else
			if(decodBuf(1)) {
#endif
				inic_conn = 0;
				resv_buf[16] = 0xfe;
				send_mes();
			}
			else {
				inic_conn = 1;
			}
			client.stop();
		}
	}
}

