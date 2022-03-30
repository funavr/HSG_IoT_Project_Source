#include <EEPROM.h>                                 // EEPROM 라이브러리 포함

void setup()
{
    Serial.begin(9600);

    int twoByte = 2022;                             // twoByte 변수 2022로 초기화

    Serial.print("twoByte value: ");
    Serial.println(twoByte);                        // twoByte 변숫값 출력
    Serial.print("Size of twoByte: ");
    Serial.println(sizeof(twoByte));                // twoByte 변수의 크기 출력

    EEPROM.write(0, twoByte);                       // 하위 1바이트를 0 주소에 쓰고
    EEPROM.write(1, twoByte >> 8);                  // 상위 1바이트를 오른쪽으로 시프트한 뒤 씀

    int readTwoByte;

    readTwoByte = EEPROM.read(0);                   // 하위 1바이트를 먼저 읽고
    readTwoByte |= EEPROM.read(1) << 8;             // 상위 1바이트를 왼쪽으로 시프트한 뒤 저장

    Serial.print("Reading two bytes data: ");
    Serial.println(readTwoByte);                    // 메모리에 쓴 값 다시 읽어 출력

    long fourByte = 20220101;                       // fourByte 변수에 4바이트 값 초기화

    Serial.print("fourByte value: ");
    Serial.println(fourByte);                       // fourByte 변숫값 출력
    Serial.print("Size of fourByte: ");
    Serial.println(sizeof(fourByte));               // fourByte 변수의 크기 출력

    EEPROM.put(10, fourByte);                       // 주소 10부터 4바이트 값 쓰기

    long readFourByte;

    EEPROM.get(10, readFourByte);                   // 10번지 주소부터 4바이트 값 읽어 저장

    Serial.print("Reading four bytes data: ");
    Serial.println(readFourByte);                   // readFourByte 변숫값 출력
}

void loop()
{

}
