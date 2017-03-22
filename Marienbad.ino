// ================================================
// Grmis @ RCGroups, march 2017
// GNU General Public Licence v3
//
// Hardware:
//  - Arduino nano
//  - SD1306 Oled DISPLAY
//  - Simple analog joystik (2 potentiometers) with one button
//
// ================================================
// I2C OLED DISPLAY: SCL = A5  ,  SDA = A4
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 14

Adafruit_SSD1306 display(OLED_RESET);
// ================================================
const int Joy_X_pin=A0; //Analog (potentiometer Joystick)
const int Joy_Y_pin=A1;
const int button_pin=2;
int Joy_X,Joy_Y;
// ================================================
static const int VarNameLength=2;
static const int NVars=8;
char VarName[NVars][VarNameLength];
int Value[NVars],Min[NVars],Max[NVars];
int present_var;
// ================================================
int final_threshold=999;//Below this number of items the computer will play it best
int nim_sum;
int NimSum(int*v) {
  int c=0;
  for (int i=0;i<NVars;i++) c=c ^ v[i];
  return c;
}
// ================================================
void Init() {
  present_var=0;
  for (int i=0;i<NVars;i++) {
    VarName[i][0]='A'+i;
    VarName[i][VarNameLength-1]=0;//end of the string
    Value[i]=2*i+1;
    Min[i]=0;
    Max[i]=20;
  }
  if (NVars>4) {
    for (int i=4;i<NVars;i++) Value[i]=0;
  }
}
// ================================================
void PrintVar(int i) {
  int&v=Value[i];
  if (v>Max[i]) v=Max[i];
  if (v<Min[i]) v=Min[i];

  display.print(VarName[i]);
  display.print(":");
  int& val=Value[i];
  if (val>=0 && val<=10) {
    for (int i=1;i<=val;i++) display.print("I");
    display.println("");
  } 
  else {
    display.println(Value[i]);
  }
}
// ================================================
void PrintGame() {
  const int nlines=8;
  display.clearDisplay();
  display.setCursor(0,0);
  int first=max(0,present_var-nlines/2);
  int last=min(first+nlines-1,NVars-1);
  first=max(0,last-nlines+1);
  for (int n=first;n<=last;n++)  {
    if (n==present_var) display.print("->"); 
    else display.print("  ");
    PrintVar(n);
  }
  display.display();
}
//===================================================
void setup() {
  // Push button pin as an input:
  pinMode(button_pin, INPUT);     
  //Game data

  Init();
  //OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
  //display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3C (for the 128x64)
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setTextWrap(true);
  display.setCursor(0,0);
  display.setTextSize(2);
  display.println("Jeu");
  display.println("   de");
  display.println("Marienbad");
  display.display();
  delay(1000);
  //===================================================
  // Choose the difficulty at startup
  int level=1;
long int start=millis();  
  const int nlevel=4;
  char* level_name[]={
    "Facile", "Moyen", "Difficile", "Expert"  };
  int level_thresh[]={
    1,2,3,999  };//Number of lines above which the computer does not play optimally 
  bool changed=true;
  while(digitalRead(button_pin)==1) {
    Joy_Y=analogRead(Joy_Y_pin);
    if (Joy_Y>800) level--,changed=true;
    if (Joy_Y<200) level++,changed=true;
    delay(100);
    if (changed) {
      if (level==nlevel) level=nlevel-1;
      if (level==-1) level=0;
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Niveau: ");
      display.println(level_name[level]);
      display.display();
      changed=false;
      delay(500); 
    } 
  }
  final_threshold=level_thresh[level];
  randomSeed(millis()-start);
  display.setTextSize(1);  
  PrintGame();
  delay(500);
}
//===================================================
void loop() {
  Joy_X=analogRead(Joy_X_pin);
  Joy_Y=analogRead(Joy_Y_pin);
  bool changed=false;
  int& pv=present_var;
  if (Joy_X>800) pv++,changed=true;
  if (Joy_X<200) pv--,changed=true;
  if (pv>=NVars) pv=0;
  if (pv<0) present_var=NVars-1;
  if (!changed) {
    if (Joy_Y<512-200) Value[pv]++,changed=true;
    if (Joy_Y>512+200) Value[pv]--,changed=true;
  }
  if (changed) {
    PrintGame();
  }
  if (digitalRead(button_pin)==0) {
    //The button has been pressed -> it is the computer's turn to play
    int i0=-1,x0=-1;  //these variables correspond to the computer's move (to be tertermined)
    //First check wether we are in the "final" part of the game,
    //that is when there is only one line with more than one item (mto)
    int one=0;//number of liens with one single item
    int lines=0;//number of nonempty lines
    int mto=0;//number of lines with more than one item
    int where=-1;
    int count=0;//total number of item(s)
    for (int i=0;i<NVars;i++) {
      count+=Value[i];
      if (Value[i]>0) lines++;
      if (Value[i]==1) one++;
      if (Value[i]>1) mto++,where=i;
    }
    if (count==1) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Bravo !");
      display.println("Vous avez gagne,");
      display.println("je dois prendre");
      display.println("la derniere...");
      display.display();
      delay(2000);
      Init();
      PrintGame();
    } 
    else {
      if (lines>final_threshold) {//Too many lines -> Play stupidly...
        //=> remove one from the longest line
        int max=0;
        for (int i=0;i<NVars;i++) {
          if (Value[i]>max) max=Value[i],i0=i;
        }
        x0=random(0,max);  
      } 
      else {//Play intellligenlty
        if (mto==1) {
          //=>Final
          if (one%2==0) {
            i0=where;
            x0=1;//Leave just one item in that unique line where there was more than 1 item
          } 
          else {
            i0=where;
            x0=0;//take all that unique line where there was more than 1 item        
          }
        } 
        else {// Not yet the final,
          if (NimSum(Value)==0) {
            //The computer is not in a safe position...
            //=> remove one from the longest line
            int max=0;
            for (int i=0;i<NVars;i++) {
              if (Value[i]>max) max=Value[i],i0=i;
            }
            x0=random(0,max);  
          } 
          else {// => try all possible moves to find one with Nimsum=0 
            int s[NVars];
            bool SafeMoveFound=false;
            for (int i=0;i<NVars && (!SafeMoveFound) ;i++) {//Consider removing something from line i  
              //copy the present configuration into the array s
              for (int j=0;j<NVars;j++) s[j]=Value[j];
              if (Value[i]>0) i0=i,x0=Value[i]-1;//Not safe, but possible move in case we do not find a better onemax/2
              for (int x=0;x<Value[i];x++) {
                s[i]=x;
                if (NimSum(s)==0) SafeMoveFound=true,i0=i,x0=x;
              }    
            }
          }
        }
      }
      if (i0>=0) {
        Value[i0]=x0;//actual move
        present_var=i0;
        PrintGame(),delay(200);
      } 
      else {
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("Vous avez perdu,");
        display.println("vous avez pris");
        display.println("la derniere !");
        display.display();
        delay(2000);
        Init();
        PrintGame();
      }
    }  
  }
  delay(300);
}
















