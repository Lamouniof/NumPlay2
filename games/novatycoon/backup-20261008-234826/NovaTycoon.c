#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint16_t color_t;
extern uint32_t np_keys(void);
extern uint32_t np_millis(void);
extern void eadk_timing_msleep(unsigned ms);
extern void eadk_display_wait_for_vblank(void);
extern void np_push(int x,int y,int w,int h,const color_t *px);
extern void np_fill_screen(int x,int y,int w,int h,color_t c);
extern void *np_alloc(uint32_t size);

#define K_LEFT 1u
#define K_RIGHT 2u
#define K_UP 4u
#define K_DOWN 8u
#define K_OK 16u
#define K_BACK 32u
#define K_HOME 64u

#define RGB(r,g,b) ((color_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))
#define C_BG RGB(8,12,22)
#define C_PANEL RGB(18,27,43)
#define C_PANEL2 RGB(25,37,57)
#define C_WHITE RGB(245,248,252)
#define C_MUTED RGB(150,165,185)
#define C_GREEN RGB(60,220,130)
#define C_RED RGB(240,80,90)
#define C_GOLD RGB(250,195,65)
#define C_BLUE RGB(65,150,245)
#define C_CYAN RGB(60,220,230)
#define C_PURPLE RGB(170,100,245)

static color_t *fb;
static int cur_tab, cur_x, cur_y, selected, sub;
static uint32_t last_tick, last_input; static bool quit_game;

static uint32_t rnd(uint32_t *s){*s^=*s<<13;*s^=*s>>17;*s^=*s<<5;return *s;}
static int clampi(int x,int a,int b){return x<a?a:x>b?b:x;}
static void rect(int x,int y,int w,int h,color_t c){if(w<=0||h<=0)return; if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;} if(x+w>320)w=320-x;if(y+h>240)h=240-y;for(int j=y;j<y+h;j++){color_t*p=fb+j*320+x;for(int i=0;i<w;i++)p[i]=c;}}
static void frame(int x,int y,int w,int h,color_t c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
static void bar(int x,int y,int w,int h,int v,int max,color_t c){rect(x,y,w,h,C_PANEL2);int q=max?((w-2)*v/max):0;rect(x+1,y+1,q,h-2,c);}

/* compact hand-made 5x7 font, digits + uppercase + symbols */
static const uint8_t glyphs[][5]={
{0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x62,0x51,0x49,0x49,0x46},{0x22,0x49,0x49,0x49,0x36},{0x18,0x14,0x12,0x7f,0x10},{0x2f,0x49,0x49,0x49,0x31},{0x3e,0x49,0x49,0x49,0x32},{1,1,0x79,5,3},{0x36,0x49,0x49,0x49,0x36},{0x26,0x49,0x49,0x49,0x3e},
{0x7e,9,9,9,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},{0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},{0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},{0,0x41,0x7f,0x41,0},{0x20,0x40,0x41,0x3f,1},{0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},{0x7f,2,4,2,0x7f},{0x7f,4,8,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},{0x7f,9,9,9,6},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{1,1,0x7f,1,1},{0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},{0x7f,0x20,0x18,0x20,0x7f},{0x63,0x14,8,0x14,0x63},{3,4,0x78,4,3},{0x61,0x51,0x49,0x45,0x43},
{0,0,0,0,0},{0,0,0x5f,0,0},{0,0x60,0x60,0,0},{0x14,0x7f,0x14,0x7f,0x14},{0x24,0x2a,0x7f,0x2a,0x12},{0x23,0x13,8,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0,5,3,0,0},{0,0x1c,0x22,0x41,0},{0,0x41,0x22,0x1c,0},{0x14,8,0x3e,8,0x14},{8,8,0x3e,8,8},{0,0x50,0x30,0,0},{8,8,8,8,8},{0,0x60,0x60,0,0},{0x20,0x10,8,4,2}
};
static int gi(char c){if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='Z')return 10+c-'A';const char*s=" -+.$%&'(),*/";for(int i=0;s[i];i++)if(s[i]==c)return 46+i;return 37;}
static void chr(int x,int y,char c,int s,color_t col){const uint8_t*g=glyphs[gi(c)];for(int i=0;i<5;i++)for(int j=0;j<7;j++)if(g[i]&(1<<j))rect(x+i*s,y+j*s,s,s,col);}
static void txt(int x,int y,const char*s,int scale,color_t c){for(;*s;s++){chr(x,y,*s,scale,c);x+=6*scale;}}
static int tw(const char*s,int sc){int n=0;while(*s++)n++;return n*6*sc;}
static void num(int x,int y,int v,int sc,color_t c){char b[16];int n=0;if(v<0){b[n++]='-';v=-v;}char t[12];int k=0;if(!v)t[k++]='0';while(v&&k<11){t[k++]=(char)('0'+v%10);v/=10;}while(k)b[n++]=t[--k];b[n]=0;txt(x,y,b,sc,c);}
static void center(int y,const char*s,int sc,color_t c){txt((320-tw(s,sc))/2,y,s,sc,c);}

static const char *names[6]={"COFFEE","ARCADE","SHOP","CINEMA","DINER","TECH"};
static const int cost0[6]={120,300,650,1200,2200,5000};
static const int rev0[6]={24,62,130,250,430,900};
static const color_t bcol[6]={RGB(175,120,80),RGB(95,120,220),RGB(80,190,120),RGB(175,90,220),RGB(220,120,70),RGB(65,190,210)};

typedef struct {int type,level,workers,marketing;bool built;} biz_t;
typedef struct {
 int cash,day,reputation,customers,weekly,expenses,company_level,selected_tab,build_type;
 int research[6], objectives[8], loans, interest, tax;
 biz_t b[20]; uint32_t seed; uint32_t last_day; int event,event_days; int popup; int total_profit,best_day;
} state_t;

static int biz_cost(const biz_t*b){int n=cost0[b->type];for(int i=1;i<b->level;i++)n+=n/2;return n;}
static int biz_income(const biz_t*b,state_t*s){int r=rev0[b->type]*(b->level*b->level);r+=b->workers*9+b->marketing*12;r=r*(90+s->reputation)/100;return r;}
static int built_count(state_t*s){int n=0;for(int i=0;i<20;i++)if(s->b[i].built)n++;return n;}
static int total_income(state_t*s){int n=0;for(int i=0;i<20;i++)if(s->b[i].built)n+=biz_income(&s->b[i],s);return n;}
static void new_company(state_t*s){
 *s=(state_t){0};s->cash=1500;s->day=1;s->reputation=55;s->company_level=1;s->seed=0xA53C91u;s->selected_tab=0;s->best_day=0;
}
static void event_roll(state_t*s){
 int r=(int)(rnd(&s->seed)%100);if(r<18){s->event=1;s->event_days=3;s->reputation=clampi(s->reputation+5,0,100);}else if(r<32){s->event=2;s->event_days=2;}else if(r<43){s->event=3;s->event_days=3;}else if(r<49){s->event=4;s->event_days=1;}else s->event=0;
}
static void advance_day(state_t*s){
 int inc=total_income(s);int exp=0;for(int i=0;i<20;i++)if(s->b[i].built)exp+=35+s->b[i].workers*11+s->b[i].marketing*7;
 int mult=100;if(s->event==1)mult=135;if(s->event==2)mult=65;if(s->event==3)mult=120;
 inc=inc*mult/100;int profit=inc-exp-s->tax; s->cash+=profit;s->weekly+=profit;s->total_profit+=profit;if(profit>s->best_day)s->best_day=profit;s->customers=inc/3+s->reputation*2;
 s->day++;if(s->event_days>0&&!--s->event_days)s->event=0;if(s->day%7==0){s->cash+=s->loans*20;s->cash-=s->loans*s->interest/10;s->tax=0;s->weekly=0;}
 if(s->day%10==0)event_roll(s);int n=built_count(s);s->company_level=1+n/3+s->reputation/25;
}
static void draw_top(state_t*s){rect(0,0,320,27,C_PANEL);txt(8,6,"NOVA TYCOON",1,C_WHITE);txt(120,6,"DAY",1,C_MUTED);num(143,6,s->day,1,C_WHITE);txt(177,6,"CASH",1,C_MUTED);num(210,6,s->cash,1,s->cash<0?C_RED:C_GREEN);rect(0,26,320,1,C_BLUE);}
static void draw_tabs(state_t*s){const char*t[5]={"CITY","BUILD","UPGRD","RESEARCH","STATS"};for(int i=0;i<5;i++){int x=i*64;rect(x,218,64,22,i==s->selected_tab?C_BLUE:C_PANEL);center(224,"",1,C_WHITE);txt(x+18,225,t[i],1,C_WHITE);}}
static void draw_city(state_t*s){
 draw_top(s);txt(8,34,"CITY OVERVIEW",2,C_WHITE);txt(232,35,"REP",1,C_MUTED);num(270,35,s->reputation,1,C_GREEN);
 for(int i=0;i<20;i++){int x=8+(i%5)*61,y=55+(i/5)*38;biz_t*b=&s->b[i];rect(x,y,56,33,b->built?bcol[b->type]:C_PANEL2);frame(x,y,56,33,b->built?C_WHITE:C_MUTED);if(b->built){txt(x+4,y+4,names[b->type],1,C_WHITE);num(x+4,y+18,b->level,1,C_GOLD);}else txt(x+18,y+12,"EMPTY",1,C_MUTED);if(i==selected){frame(x-2,y-2,60,37,C_CYAN);}}
 txt(8,208,"ARROWS SELECT   OK MANAGE",1,C_MUTED);draw_tabs(s);
}
static void draw_build(state_t*s){
 draw_top(s);txt(8,34,"BUILD NEW BUSINESS",2,C_WHITE);txt(8,52,"SELECT AN EMPTY PLOT",1,C_MUTED);
 for(int i=0;i<6;i++){int x=8+(i%3)*104,y=70+(i/3)*45;bool on=i==s->build_type;rect(x,y,96,38,on?C_PANEL2:C_PANEL);frame(x,y,96,38,on?C_CYAN:C_MUTED);txt(x+5,y+5,names[i],1,C_WHITE);txt(x+5,y+19,"COST",1,C_MUTED);num(x+35,y+19,cost0[i],1,C_GOLD);}
 txt(8,170,"PLOT",1,C_MUTED);num(42,170,selected+1,1,C_WHITE);txt(72,170,"PRICE",1,C_MUTED);num(110,170,cost0[s->build_type],1,C_GOLD);
 txt(8,190,"UP/DOWN TYPE   LEFT/RIGHT PLOT",1,C_MUTED);txt(8,203,"OK BUILD   BACK CITY",1,C_MUTED);draw_tabs(s);
}
static void draw_upgrade(state_t*s){
 draw_top(s);txt(8,34,"MANAGEMENT",2,C_WHITE);biz_t*b=&s->b[selected];if(!b->built){center(90,"SELECT A BUSINESS",2,C_MUTED);draw_tabs(s);return;}
 txt(8,58,names[b->type],2,bcol[b->type]);txt(8,78,"LEVEL",1,C_MUTED);num(50,78,b->level,1,C_WHITE);txt(100,78,"INCOME/DAY",1,C_MUTED);num(174,78,biz_income(b,s),1,C_GREEN);
 txt(8,100,"WORKERS",1,C_MUTED);num(60,100,b->workers,1,C_WHITE);txt(105,100,"MARKETING",1,C_MUTED);num(180,100,b->marketing,1,C_WHITE);
 rect(8,122,146,28,sub==0?C_BLUE:C_PANEL2);txt(18,132,"UPGRADE",1,C_WHITE);num(96,132,biz_cost(b),1,C_GOLD);
 rect(164,122,146,28,sub==1?C_BLUE:C_PANEL2);txt(174,132,"HIRE +1",1,C_WHITE);num(260,132,35+b->workers*12,1,C_GOLD);
 rect(8,156,146,28,sub==2?C_BLUE:C_PANEL2);txt(18,166,"MARKET +1",1,C_WHITE);num(96,166,50+b->marketing*25,1,C_GOLD);
 rect(164,156,146,28,sub==3?C_BLUE:C_PANEL2);txt(174,166,"SELL",1,C_WHITE);num(260,166,biz_cost(b)/2,1,C_RED);
 txt(8,194,"UP/DOWN ACTION   OK CONFIRM",1,C_MUTED);draw_tabs(s);
}
static void draw_research(state_t*s){
 draw_top(s);txt(8,34,"RESEARCH LAB",2,C_WHITE);const char*rn[6]={"LOYALTY","AUTOMATION","PREMIUM","LOGISTICS","ANALYTICS","MEGASTORE"};for(int i=0;i<6;i++){int x=8+(i%2)*154,y=62+(i/2)*45;int cost=300+i*350;rect(x,y,146,37,sub==i?C_BLUE:C_PANEL);txt(x+6,y+5,rn[i],1,C_WHITE);txt(x+6,y+20,"LVL",1,C_MUTED);num(x+35,y+20,s->research[i],1,C_GOLD);num(x+92,y+20,cost,1,C_WHITE);}
 txt(8,202,"ARROWS CHOOSE   OK INVEST",1,C_MUTED);draw_tabs(s);
}
static void draw_stats(state_t*s){
 draw_top(s);txt(8,34,"COMPANY REPORT",2,C_WHITE);txt(8,62,"DAILY REVENUE",1,C_MUTED);num(112,62,total_income(s),1,C_GREEN);txt(8,82,"DAILY EXPENSE",1,C_MUTED);num(112,82,s->expenses,1,C_RED);txt(8,102,"CUSTOMERS",1,C_MUTED);num(112,102,s->customers,1,C_WHITE);txt(8,122,"PROFIT RECORD",1,C_MUTED);num(112,122,s->best_day,1,C_GOLD);txt(8,142,"BUSINESSES",1,C_MUTED);num(112,142,built_count(s),1,C_WHITE);txt(8,162,"COMPANY LEVEL",1,C_MUTED);num(112,162,s->company_level,1,C_PURPLE);txt(170,62,"WEEKLY PROFIT",1,C_MUTED);num(170,82,s->weekly,1,s->weekly>=0?C_GREEN:C_RED);txt(170,110,"TOTAL PROFIT",1,C_MUTED);num(170,130,s->total_profit,1,C_GOLD);txt(170,158,"LOANS",1,C_MUTED);num(170,178,s->loans,1,C_WHITE);draw_tabs(s);
}
static void draw_event(state_t*s){if(!s->event)return;rect(35,72,250,92,C_PANEL);frame(35,72,250,92,C_GOLD);const char*a=s->event==1?"CITY FESTIVAL":s->event==2?"MARKET SLUMP":s->event==3?"VIRAL MOMENT":"SUPPLY TAX";const char*b=s->event==1?"CUSTOMERS SURGE":s->event==2?"SALES SLOW DOWN":s->event==3?"REPUTATION BOOST":"EXTRA COST TODAY";center(86,a,2,C_GOLD);center(116,b,1,C_WHITE);center(140,"EFFECT ACTIVE",1,C_MUTED);}
static void draw(state_t*s){rect(0,0,320,240,C_BG);if(s->selected_tab==0)draw_city(s);else if(s->selected_tab==1)draw_build(s);else if(s->selected_tab==2)draw_upgrade(s);else if(s->selected_tab==3)draw_research(s);else draw_stats(s);if(s->event)draw_event(s);}
static void build_here(state_t*s){if(selected<0||selected>=20||s->b[selected].built)return;int t=s->build_type;if(s->cash<cost0[t])return;s->cash-=cost0[t];s->b[selected]=(biz_t){t,1,1,0,true};s->reputation=clampi(s->reputation+2,0,100);}
static void manage_ok(state_t*s){biz_t*b=&s->b[selected];if(!b->built)return;int c=0;if(sub==0){c=biz_cost(b);if(s->cash>=c){s->cash-=c;b->level++;s->reputation=clampi(s->reputation+1,0,100);}}else if(sub==1){c=35+b->workers*12;if(s->cash>=c&&b->workers<9){s->cash-=c;b->workers++;}}else if(sub==2){c=50+b->marketing*25;if(s->cash>=c&&b->marketing<8){s->cash-=c;b->marketing++;}}else{c=biz_cost(b)/2;s->cash+=c;s->reputation=clampi(s->reputation-3,0,100);*b=(biz_t){0};}}
static void research_ok(state_t*s){int i=sub;if(s->research[i]>=5)return;int c=300+i*350+s->research[i]*500;if(s->cash<c)return;s->cash-=c;s->research[i]++;s->reputation=clampi(s->reputation+2,0,100);}
static void input(state_t*s){uint32_t k=np_keys();if(!k)return;if(np_millis()-last_input<130)return;last_input=np_millis();
 if(k&K_HOME){quit_game=true;return;}if(k&K_BACK){if(s->event){s->event=0;return;}if(s->selected_tab==0){s->selected_tab=1;sub=0;return;}s->selected_tab=0;return;}
 if(s->selected_tab==0){if(k&K_BACK){s->selected_tab=1;sub=0;return;}if(k&K_LEFT)selected=selected%5?selected-1:selected;else if(k&K_RIGHT)selected=selected%5<4?selected+1:selected;else if(k&K_UP)selected=selected>=5?selected-5:selected;else if(k&K_DOWN)selected<15?selected+5:selected;else if(k&K_OK){s->selected_tab=s->b[selected].built?2:1;sub=0;}}
 else if(s->selected_tab==1){if(k&K_UP)s->build_type=(s->build_type+5)%6;else if(k&K_DOWN)s->build_type=(s->build_type+1)%6;else if(k&K_LEFT)selected=selected%5?selected-1:selected;else if(k&K_RIGHT)selected=selected%5<4?selected+1:selected;else if(k&K_OK)build_here(s);}
 else if(s->selected_tab==2){if(k&K_UP)sub=(sub+3)%4;else if(k&K_DOWN)sub=(sub+1)%4;else if(k&K_LEFT)selected=selected%5?selected-1:selected;else if(k&K_RIGHT)selected=selected%5<4?selected+1:selected;else if(k&K_OK)manage_ok(s);}
 else if(s->selected_tab==3){if(k&K_UP)sub=(sub+5)%6;else if(k&K_DOWN)sub=(sub+1)%6;else if(k&K_OK)research_ok(s);}
 else {if(k&K_LEFT)s->selected_tab=3;else if(k&K_RIGHT)s->selected_tab=0;}
 if(k&K_LEFT&&s->selected_tab>0&&s->selected_tab!=2&&s->selected_tab!=1)s->selected_tab--;if(k&K_RIGHT&&s->selected_tab<4&&s->selected_tab!=2&&s->selected_tab!=1)s->selected_tab++;
}
int main(void){
 fb=(color_t*)np_alloc(320*240*2);if(!fb)return 0;state_t s;new_company(&s);selected=0;sub=0;last_input=0;last_tick=np_millis();
 while(1){uint32_t now=np_millis();input(&s);if(quit_game)break;if(now-last_tick>=1000){last_tick+=1000;advance_day(&s);}s.expenses=0;for(int i=0;i<20;i++)if(s.b[i].built)s.expenses+=35+s.b[i].workers*11+s.b[i].marketing*7;draw(&s);np_push(0,0,320,240,fb);if(np_keys()&K_BACK){eadk_timing_msleep(120);if(np_keys()&K_BACK)break;}eadk_display_wait_for_vblank();}
 np_wait_release: ;
 return 0;
}
