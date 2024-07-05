#include "pitches.h"  //載入音調和頻率的對照檔
#define TONE_PIN 11 // 定義蜂鳴器腳位
#define NUMBER 3 // 3個LED、3個開關

#include <Wire.h> // I2C程式庫
#include <LiquidCrystal_I2C.h> // LCD_I2C模組程式庫
LiquidCrystal_I2C lcd(0x27, 16, 2); //2 * 16 且參數 0x27 是液晶顯示屏的 I2C 位址
 
// 定義開關與LED的腳位
const int switches[NUMBER] = {3,2,1}; //按鈕
const int leds[NUMBER] = {10,9,8}; //LED燈
int led1 = 13; //答對燈的腳位
int led2 = 12;
 
// 定義各LED對應的音符頻率
const int notes[NUMBER] = {
  NOTE_C4, NOTE_D4, NOTE_E4
};
 
typedef struct{
  int *note;  //存儲音符的頻率
  int *duration; //存儲每個音符的持續時間
  int number;  //旋律中的音符數量
}Melody;
 
typedef enum{
  MELODY_START,  //開始音樂旋律
  MELODY_CORRECT,
  MELODY_WRONG,
  MELODY_MAX,  //列舉的範圍
} Melody_Enum;
 
typedef enum{
  STATE_START,
  STATE_QUESTION, // 重新開始遊戲
  STATE_ANSWER,  // 等待使用者按下開關，回答問題
  STATE_CORRECT, // 正確，播放答對聲音，重置
  STATE_WRONG,    // 錯誤，播放答錯聲音，重置
  STATE_MAX,
} State;
 

int q_num = 3;
int *questions = NULL; //儲問題的陣列指標
int *answers = NULL; //存儲回答的陣列
int answer_num = 0;  //回答的數量
unsigned long lastClickTime; //存儲上一次按下開關的時間
 
State state = STATE_START; //表示當前的狀態，初始值為開始狀態


//起始音效
int noteStart[] = {
  NOTE_C4, NOTE_F4, NOTE_C4, NOTE_F4, NOTE_C4, NOTE_F4, NOTE_C4,
  NOTE_F4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_F4, NOTE_G4
};
int noteDurationsStart[] = {16, 8, 16, 8, 16, 4, 16, 16, 16, 16, 8, 16, 4};
//答對音效
int noteCorrect[] = {NOTE_C4, NOTE_C4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5};
int noteDurationsCorrect[] = {16, 16, 16, 8, 16, 8};
//答錯音效
int noteWrong[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, NOTE_C5, NOTE_B3, NOTE_C4};
int noteDurationsWrong[] = {16, 32, 32, 16, 16, 16, 16, 16};
 
Melody melodys[MELODY_MAX] = {
  {noteStart, noteDurationsStart, 13}, //音符頻率和持續時間，共有 13 個音符
  {noteCorrect, noteDurationsCorrect, 6},
  {noteWrong, noteDurationsWrong, 8},
};

//用於播放音樂 
void playtone(int *note, int *noteDurations, int num){
// 透過迴圈依次播放每個音符
  for(int thisNote = 0; thisNote < num; thisNote++){
// 計算音符的持續時間
    int noteDuration = 3000 / noteDurations[thisNote];
// 使用 tone() 函式播放音符
    tone(TONE_PIN, note[thisNote], noteDuration);
// 計算音符之間的暫停時間
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
  }
}
 
void playMelody(Melody_Enum me){
  playtone(melodys[me].note, melodys[me].duration, melodys[me].number);
}
// 初始化開關的腳位，所以預設狀態會是HIGH，
// 之後讀取開關狀態時，若讀到LOW，代表按下按鍵
void setup(){
  for(int i = 0; i < NUMBER; i++){
    pinMode(switches[i], INPUT);
    digitalWrite(switches[i], HIGH);
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }
 
  randomSeed(analogRead(A1));    
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
 
  lcd.init();    // 初始化LCD
  lcd.backlight();
  Serial.begin(115200);  
}

// 釋放問題和答案陣列的記憶體並將指標設置為空 
void reset(){    
  free(questions);
  questions = NULL;  
  answer_num = 0;
  free(answers);
  answers = NULL;
 
  for(int i = 0; i < NUMBER; i++){
    digitalWrite(leds[i], LOW);
  }
}
 
void playOneTone(int note, float delayScale){   // 只播放一個音符的函式，秀出問題時可用，// 使用者回答時也可用。
  int noteDuration = 3000 / 8;  // 計算音符的持續時間
    tone(TONE_PIN, note, noteDuration); // 播放音符
 
    int pauseBetweenNotes = noteDuration * delayScale;
    delay(pauseBetweenNotes);
}
void playQuestionsTone(){    // 準備好問題後，放出音效、閃爍LED  //這邊也需要改
  for(int i = 0; i < q_num; i++){    
    digitalWrite(leds[questions[i]], HIGH); // 點亮對應問題的 LED
    playOneTone(notes[questions[i]], 1.3);  // 播放問題音符
    digitalWrite(leds[questions[i]], LOW); // 關閉 LED
  }
}
 
boolean check(){     // 檢查問題跟使用者的回答是不是一樣
  for(int i = 0; i < q_num; i++){
    if(questions[i] != answers[i]){
      return false; // 如果有任何一個答案不一致，返回 false
    }
  }
  return true;
}
 
void LCD_show(){
  lcd.setCursor(2, 0); // (colum, row)從第一排的第六個位置開始顯示  
  lcd.print("  Welcome");
  lcd.setCursor(2, 1); // (colum,row)從第二排第六格位置開始顯示
  lcd.print("  Player!");
  delay(1000);
  lcd.setCursor(2, 0); 
  lcd.print("          ");
  lcd.setCursor(2, 1); 
  lcd.print("          ");
  delay(1000);
  lcd.setCursor(2, 0); // (colum, row)從第一排的第三個位置開始顯示
  lcd.print("Are you ready");  
  lcd.setCursor(2, 1); // (colum,row)從第二排第三格位置開始顯示
  lcd.print(" to Start?");
  delay(3000);
  lcd.setCursor(2, 0); // (colum, row)從第一排的第三個位置開始顯示
  lcd.print("              ");
  lcd.setCursor(2, 1); // (colum,row)從第二排第三格位置開始顯示
  lcd.print("            ");
  delay(1000);
}
void LED_W(){
    lcd.setCursor(2, 0); // (colum, row)從第一排的第三個位置開始顯示
    lcd.print("   OOPS"); 
    lcd.setCursor(2, 1); // 
    lcd.print(" You loose");
    delay(1000);
    lcd.setCursor(2, 0); // (colum, row)從第一排的第三個位置開始顯示
    lcd.print("              "); 
    lcd.setCursor(2, 1); // (colum,row)從第二排第三格位置開始顯示
    lcd.print("            ");
    delay(1000);
}
void LED_Q(){ 
    lcd.setCursor(2, 1); // (colum,row)從第二排第三格位置開始顯示
    lcd.print("      Next???");
    delay(2000);
    lcd.setCursor(2, 1); // (colum,row)從第二排第三格位置開始顯示
    lcd.print("              ");
    delay(2000);
}
void LED_R(){
    lcd.setCursor(2, 0); // (colum, row)從第一排的第三個位置開始顯示
    lcd.print("You pass!?"); 
    lcd.setCursor(2, 1); // (colum,row)從第二排第三格位置開始顯示
    lcd.print("Excellent");
    delay(1000);
    lcd.setCursor(2, 0); // (colum, row)從第一排的第三個位置開始顯示
    lcd.print("              ");     
    lcd.setCursor(2, 1); // (colum, row)從第一排的第三個位置開始顯示
    lcd.print("              "); 
    delay(2000);
}
void writeToSerial(){
  static  unsigned long count = 1;
  lcd.print(count); // 從0開始輸出，每次加1
  count++;  
} 
 
void loop(){

  switch(state){      // 以state記錄目前狀態，根據狀態作事
    case STATE_START:{    // 開始遊戲，播放開始音效，進入問題狀態 
      LCD_show(); // 顯示歡迎畫面
      reset(); // 重置問題和答案
      playMelody(MELODY_START); // 播放開始音效
      state = STATE_QUESTION; // 進入問題狀態
      break;
    }
   
    case STATE_QUESTION:{     // 問題狀態，以亂數製作問題
      questions = (int *)(malloc(sizeof(int) * q_num)); //問題陣列
      answers = (int *)(malloc(sizeof(int) * q_num)); //答案陣列
      for(int i = 0; i < q_num; i++){
        questions[i] = random(0, NUMBER);
      }
      answer_num = 0;
      playQuestionsTone();  // 顯示問題
      lastClickTime = millis();  // 記錄目前時間
      state = STATE_ANSWER;
      break;
    }
   
    case STATE_ANSWER:{
      const unsigned long nowTime = millis();   // 看看使用者多久沒按開關了，超過5秒就宣布失敗
      if(nowTime >= lastClickTime + 5000UL){
        state = STATE_WRONG;
        break;
      }
     
      for(int i = 0; i < NUMBER; i++){     // 讀取每個開關的狀態
        int ss = digitalRead(switches[i]); // 讀取開關的狀態
        if(ss == LOW){ // 如果開關被按下
          digitalWrite(leds[i], HIGH); // 點亮對應的 LED
          lastClickTime = nowTime;  // 更新最後按下開關的時間
          answers[answer_num] = i;   // 把使用者的輸入答案存起來
          answer_num++;
          playOneTone(notes[i], 1);  // 播放對應的音符
          digitalWrite(leds[i], LOW); // 熄滅 LED
          delay(200);
          break;
        }
       
      }
     
      if(answer_num >= q_num){    // 判斷是不是已經回答完了
        state = check() ? STATE_CORRECT : STATE_WRONG;
      }
      break;
    }
   
    case STATE_CORRECT:{      // 回答正確
      q_num++;
      writeToSerial(); // 將結果寫入串行端口    
      digitalWrite(led1, HIGH); // 點亮 led1
      playMelody(MELODY_CORRECT);
      delay(2000);
      digitalWrite(led1, LOW);
      state = STATE_START; // 回到開始狀態
      LED_R(); // 顯示答對提示
      break;
    }
   
    case STATE_WRONG:{   //回答錯誤
      LED_W();
      digitalWrite(led2, HIGH);
      playMelody(MELODY_WRONG);
      delay(2000);
      digitalWrite(led2, LOW);
      state = STATE_START; // 回到開始狀態
      LED_Q(); // 顯示下一步提示
      break;
    }
   
    default:{
      state = STATE_START; // 預設情況下，回到開始狀態
      break;
    }
  }
}


