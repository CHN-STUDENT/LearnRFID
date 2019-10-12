#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN); //实例化类
// 初始化数组用于存储读取到的NUID 
byte nuidPICC[4];
//主卡ID数组
byte masterCardID[4]={0x59,0x94,0xC1,0x8E};
byte deleteCardID[4]={0x5B,0x63,0x43,0x0A};
//蓝卡ID结构体
struct CardStruct {
  byte nuidPICC[4];
};
int index = 1; //当前蓝卡索引值
const int number = 4;//蓝卡数量
int startAddress = 0; //EERPOM 开始地址
struct CardStruct card[number+1]; //蓝卡结构体数组
bool flag = false; //是否刷了白卡
bool deleteFlag = false; //是否刷了删除卡

void setup() {
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  Serial.begin(9600);
  SPI.begin(); // 初始化SPI总线
  rfid.PCD_Init(); // 初始化 MFRC522 
  readEEPROM(); //读取存储的蓝卡值
}

void loop() {
  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  // 验证NUID是否可读
  if ( ! rfid.PICC_ReadCardSerial())
    return;
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  // 检查是否MIFARE卡类型
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println("不支持读取此卡类型");
    return;
  }
  // 将NUID保存到nuidPICC数组
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }   
  if(isMaster(nuidPICC)==true) {
    if(flag==true) {
      Serial.println("白卡已刷，关闭蓝卡写入");
      flag = false;
    } else {
      Serial.println("白卡已刷，开启蓝卡写入");
      flag = true;
    }
  } else if(isDelete(nuidPICC)==true) {
     index=1;
     for (int i = 0 ; i < EEPROM.length() ; i++) {
       EEPROM.write(i, 0);
     }
     for(int i = 1;i <= number; i++) {
       card[i].nuidPICC[0]=0x00;
       card[i].nuidPICC[1]=0x00;
       card[i].nuidPICC[2]=0x00;
       card[i].nuidPICC[3]=0x00;
     }
     
  } else {
    if(flag==true) {
      if(index<=number) {
        card[index].nuidPICC[0]=nuidPICC[0];
        card[index].nuidPICC[1]=nuidPICC[1];
        card[index].nuidPICC[2]=nuidPICC[2];
        card[index].nuidPICC[3]=nuidPICC[3];
        EEPROM.put(startAddress+(number-1)*sizeof(CardStruct),card[index]);
        Serial.println("存储位"+String(index)+"蓝卡写入，写入值");
        Serial.print("十六进制UID：");
        printHex(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();
        Serial.print("十进制UID：");
        printDec(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();
      }  else {
        Serial.println("EEPROM可用存储耗尽，无法再写入");
        Serial.println("请刷删除卡之后清除数据");
        //index = 1;
      }
       index++;
       delay(2000);
    } else {
      if(isSaveCard(nuidPICC)==-1){
         Serial.println("请刷白卡以便再次开启蓝卡写入");
      }
    }
  }
   // 使放置在读卡区的IC卡进入休眠状态，不再重复读卡
  rfid.PICC_HaltA();
   // 停止读卡模块编码
  rfid.PCD_StopCrypto1();
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : "");
    Serial.print(buffer[i], HEX);
  }
}
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : "");
    Serial.print(buffer[i], DEC);
  }
}

void readEEPROM()
{
  for(int i=0;i<number;i++){
      EEPROM.get(startAddress+i*sizeof(CardStruct),card[i+1]);
  }
}

int isSaveCard(byte *cradID){ 
  bool flag=false;
  for(int i=1;i<=number;i++) {
     if(cradID[0]==card[i].nuidPICC[0]&&
      cradID[1]==card[i].nuidPICC[1]&&
      cradID[2]==card[i].nuidPICC[2]&&
      cradID[3]==card[i].nuidPICC[3]
      ){
        Serial.println("此卡为第 "+String(index)+" 蓝卡，蓝灯亮");
        digitalWrite(6,HIGH);
        delay(2000);
        digitalWrite(6,LOW);
        delay(300);
        flag=true;
        return i;
      }
  }
  if(flag==false) {
    Serial.println("此卡为未知卡，白灯亮");
    digitalWrite(4,HIGH);
    delay(2000);
    digitalWrite(4,LOW);
    return -1;
  }//白灯
}


bool isDelete(byte *cardID) {
  if(cardID[0]==deleteCardID[0]&&
  cardID[1]==deleteCardID[1]&&
  cardID[2]==deleteCardID[2]&&
  cardID[3]==deleteCardID[3]
  ){
    Serial.println("此卡为删除卡，删除所有数据！");
    digitalWrite(4,HIGH);
    digitalWrite(5,HIGH);
    digitalWrite(6,HIGH);
    delay(2000);
    digitalWrite(4,LOW);
    digitalWrite(5,LOW);
    digitalWrite(6,LOW);
    delay(300);
    return true;
  } else {
    return false;
  }
}

bool isMaster(byte *cradID){
  if(cradID[0]==masterCardID[0]&&
  cradID[1]==masterCardID[1]&&
  cradID[2]==masterCardID[2]&&
  cradID[3]==masterCardID[3]
  ){
    //Serial.println("此卡为白卡，红灯亮");
    digitalWrite(5,HIGH);
    delay(2000);
    digitalWrite(5,LOW);
    delay(300);
    return true;
  } else {
    return false;
  }
}
