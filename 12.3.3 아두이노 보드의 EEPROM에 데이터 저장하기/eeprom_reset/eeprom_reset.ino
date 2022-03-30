#include <EEPROM.h> // EEPROM 표준 라이브러리 포함

void setup() 
{
    Serial.begin(9600);

    Serial.println("EEPROM RESET");

    unsigned long sumOfMemory = 0;

    for (int i = 0; i < EEPROM.length(); i++)// EEPROM의 크기 만큼 반복
    {
        sumOfMemory += EEPROM.read(i);// EEPROM의 값을 읽어 누적 저장
    }

    Serial.println("Before RESET Sum of Memory: ");
    Serial.println(sumOfMemory);

    for (int i = 0; i < EEPROM.length(); i++)// EEPROM 크기만큼 반복
    {
        EEPROM.write(i, 0);// EEPROM 주소에 0을 씀, 초기화
    }

    sumOfMemory = 0;// sumOfMemory 변숫값 0으로 초기화
    
    Serial.println("After RESET");
    for (int i = 0; i < EEPROM.length(); i++)// EEPROM 크기만큼 반복
    {
        sumOfMemory += EEPROM.read(i);// EEPROM의 값을 읽어 누적 저장
    }

    Serial.println("After RESET Sum of Memory: ");
    Serial.println(sumOfMemory);
}

void loop() 
{

}
