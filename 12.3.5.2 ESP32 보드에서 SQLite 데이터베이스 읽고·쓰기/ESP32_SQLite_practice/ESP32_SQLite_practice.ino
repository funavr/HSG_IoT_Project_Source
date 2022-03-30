#include <sqlite3.h>                 // SQLite3 DB 라이브러리 포함
#include <SPI.h>                // SPI 통신 표준 라이브러리 포함
#include <FS.h>                 // 파일 시스템 라이브러리 포함
#include <SPIFFS.h>                 // SPIFFS 라이브러리 포함

const char* data = "CallBack function called";  // 콜백 함수 호출 문장 저장

// 콜백 함수 메시지, 데이터 갯수, 데이터 항목, 데이터 항목의 분류명
static int CallBack(void *data, int argc, char **argv, char **azColName)
{
    Serial.printf("%s: ", (const char*)data);
    for(int i = 0; i < argc; i++) 
    {
        Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    Serial.printf("\n");
    
    return 0;
}

int DBOpen(const char *filename, sqlite3 **db)  // 데이터베이스 파일을 여는 함수
{
    int rc = sqlite3_open(filename, db);

    if(rc)                  // 결과가 0이 아니면, DB 오픈 실패
    {
        Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
        return rc;
    }
    else                    // 결과가 0이면 DB 오픈 성공
    {
        Serial.printf("Opened database successfully\n");
    }

    return rc;
}

char *zErrMsg;                  // 오류 메시지 문자열 저장 포인터

int DBExec(sqlite3 *db, const char *sql)        // SQL 명령문 실행 함수
{
    Serial.println(sql);

    long startCmdTime = micros();       // 명령 실행 시간 기록
    int rc = sqlite3_exec(db, sql, CallBack, (void*)data, &zErrMsg);

    if(rc != SQLITE_OK)             // 오류이면,
    {
        Serial.printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        Serial.printf("Operation done successfully\n");
    }

    Serial.print("Time taken:");
    Serial.println(micros() - startCmdTime);    // 명령문 실행 시간 계산 후 출력

    return rc;
}

void setup()
{
    Serial.begin(115200);
    sqlite3 *practiceDB;                // 데이터베이스 객체 포인터

    if(!SPIFFS.begin(true))             // SPIFFS 초기화 또는 시작 안되면,
    {
        Serial.println("ESP32 System Error: Can not use SPIFFS");
        return;                     // 종료
    }

    File root = SPIFFS.open("/");       // SPIFFS 열어 root에 저장

    if(!root)                   // 파일 객체가 null이면,
    {
        Serial.println("ESP32 System Error: Can not use File");
        return;                     // 종료
    }

    root.close();               // 파일 객체 닫기

    File dbFile = SPIFFS.open("/practice.db");  // SPIFFS practice.db 파일 오픈

    if(!dbFile)                     // practice.db 파일을 열 수 없으면
    {
        Serial.println("SQLite Error: Can not Open sqlite db file");
        return;                     // 종료
    }

    // practice.db 파일을 열 수 있음을 확인하고 파일을 닫음
    dbFile.close();

    sqlite3_initialize();               // SQLite 라이브러리 초기화

    if(DBOpen("/spiffs/practice.db", &practiceDB))
    {                           // 파일 열기에 실패하면
        Serial.println("SQLite Can not Open DB");
        return;                     // 종료
    }

    int rc;                 

    // 테이블에 id가 1이고, name이 'Lucy'인 개체 추가
    rc = DBExec(practiceDB, "INSERT INTO test VALUES (1, 'Lucy');");

    if(rc != SQLITE_OK)             // 오류가 없음이 아니라면, 즉 오류면
    {
        sqlite3_close(practiceDB);
        return;                 // 종료
    }

    // 테이블에 id가 2이고, name이 'Mary'인 개체 추가
    rc = DBExec(practiceDB, "INSERT INTO test VALUES (2, 'Mary');");

    if(rc != SQLITE_OK) 
    {
        sqlite3_close(practiceDB);
        return;
    }

    rc = DBExec(practiceDB, "SELECT * FROM test"); 

    if(rc != SQLITE_OK)
    {
        sqlite3_close(practiceDB);
        return;
    }
}

void loop()
{

}
