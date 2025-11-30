#include <reg51.h>
#include <intrins.h>
#include <stdio.h>
#include <string.h>

#define uchar unsigned char
#define uint unsigned int

sbit RS = P3^6;
sbit RW = P3^5;
sbit E  = P3^4;

// 蜂鸣器
sbit BEEP = P3^7;

//按键
sbit L0 = P2^4;
sbit L1 = P2^5;
sbit L2 = P2^6;
sbit L3 = P2^7;

//全局变量
uchar default_password[] = "123456";
uchar input_password[7]; 
uchar input_index = 0; 
uchar new_password[7];
uchar admin_mode = 0;
uchar password_step = 0; 
uchar error_count = 0;
uchar locked = 0;
uint lock_timer = 0;
lock_seconds = 0;
//声明
void delay1ms();
void Check_Busy_LCD();
void Write_Cmd_LCD(uchar cmd);
void Write_Data_LCD(uchar dat);
void Init_LCD();
void Write_Str_LCD(uchar addr, char *str);
void Write_Char_LCD(uchar addr, uchar dat);
void Clear_Line(uchar line);
uchar Get_Key();
void Beep(uchar type);

//延时函数
void delay1ms() {
    uint i = 1000;
    while(i--) _nop_();
}

void delayms(uint time) {
    uint i;
    while(time--) { for(i=0;i<80;i++); }
}

// LCD判断占用
void Check_Busy_LCD() {
    uchar temp;
    do {
        E = 0;
        RS = 0;
        RW = 1;
        E = 1;
        temp = P1;
    } while(temp & 0x80);
    E = 0;
}

// 写命令
void Write_Cmd_LCD(uchar cmd) {
    Check_Busy_LCD();
    E = 0;
    RS = 0;
    RW = 0;
    P1 = cmd;
    E = 1;
    delay1ms();
    E = 0;
    delay1ms();
}

//写数据
void Write_Data_LCD(uchar dat) {
    Check_Busy_LCD();
    E = 0;
    RS = 1;
    RW = 0;
    P1 = dat;
    E = 1;
    delay1ms();
    E = 0;
    delay1ms();
}

// 初始化
void Init_LCD() {
    Write_Cmd_LCD(0x38); //
    Write_Cmd_LCD(0x0c); //
    Write_Cmd_LCD(0x06); //
    Write_Cmd_LCD(0x01); //
}

//写字符
void Write_Char_LCD(uchar addr, uchar dat) {
    Write_Cmd_LCD(addr);
    Write_Data_LCD(dat);
}

//写字符串
void Write_Str_LCD(uchar addr, char *str) {
    Write_Cmd_LCD(addr);
    while(*str > 0) Write_Data_LCD(*str++);
}



//清空行
void Clear_Line(uchar line) {
    uchar i;
    uchar addr = (line == 1) ? 0x80 : 0xC0;
    Write_Cmd_LCD(addr);
    for(i = 0; i < 16; i++) Write_Data_LCD(' ');
}

//按键矩阵定义
/* 0 1 2 3 */
/* 4 5 6 7 */
/* 8 9 10(A) 11(B) */
/* 12(√) 13(C) 14(D) 15(*) */
uchar Get_Key() {
    uchar temp, num = 0xFF;
    uchar i;

    P2 = 0xf0;
    if((P2 & 0xf0) == 0xf0) return 0xFF; 

    delayms(100);
    if((P2 & 0xf0) != 0xf0) {
        temp = 0xfe;
        for(i = 0; i < 4; i++) {
            P2 = temp;
            delayms(1);
            if(L0 == 0) num = (i << 2);
            if(L1 == 0) num = (i << 2) + 1;
            if(L2 == 0) num = (i << 2) + 2;
            if(L3 == 0) num = (i << 2) + 3;
            temp = _crol_(temp, 1);
            temp = temp | 0xf0;
        }
    }
    while((P2 & 0xf0) != 0xf0);
    return num;
}

//蜂鸣器
void Beep(uchar type) {
		uint i;
    if(type == 1) {
        for(i = 0; i < 500; i++) {
            BEEP = ~BEEP;
            delayms(1);
        }
    } else if(type == 2) {
        for(i = 0; i < 200; i++) {
            BEEP = ~BEEP;
            delayms(2);
        }
    }
    BEEP = 1; //
}
//重置系统
void Reset_System() {
    input_index = 0;
    admin_mode = 0;
    password_step = 0;
    memset(input_password, 0, sizeof(input_password));
    memset(new_password, 0, sizeof(new_password));
    Clear_Line(1);
    Clear_Line(2);
    Write_Str_LCD(0x80, "Enter Password:");
}
void main() {
		uint system_timer = 0;
		uint delay_count = 0;
    uchar key;
    Init_LCD();
    Write_Str_LCD(0x80, "Enter Password:");
    // 
    TMOD = 0x01;  
    TH0 = 0xFC;  
    TL0 = 0x66;
    TR0 = 1;    
    ET0 = 1;     
    EA = 1;    
		//lock_seconds = 5;
    while(1) {
        key = Get_Key();
				//检查锁定
        if(locked) {
            if(delay_count >= 1000) { 
                delay_count = 0;
                lock_seconds--;
                //
                Clear_Line(1);
                Write_Str_LCD(0x80, "Locked!");
                Clear_Line(2);
                sprintf(input_password, "Wait %d seconds", lock_seconds);
                Write_Str_LCD(0xC0, input_password);
                
                if(lock_seconds <= 0) {
                    locked = 0;
                    error_count = 0;
                    Clear_Line(1);
                    Clear_Line(2);
                    Write_Str_LCD(0x80, "Unlocked!");
                    Write_Str_LCD(0xC0, "Enter Password:");
                    delayms(100);
                    Reset_System();
                }
            }
            delay_count++;
            delayms(1);
            continue; // 跳出按键
        }
				//******//
        if(key != 0xFF && !locked) {
            if(key < 10) { // 0123456789
                if(input_index < 6) {
                    if(admin_mode == 0) {
                        // 客户模式
                        input_password[input_index++] = key + '0';
                        Write_Char_LCD(0xC0 + 3 + input_index, '*');
                    } else if(admin_mode == 1 || admin_mode == 2) {
                        // 管理员模式
                        input_password[input_index++] = key + '0';
                        Write_Char_LCD(0xC0 + 3 + input_index, '*');
                    }
                }
            } else if(key == 12) { //确认
                input_password[input_index] = '\0';
                
                if(admin_mode == 0) {
                    // 
                    if(strcmp(input_password, default_password) == 0) {
                        Clear_Line(1);
                        Write_Str_LCD(0x80, "Welcome!");
                        Beep(1);
												error_count = 0;//密码正确重置错误计数
                    } else {
                        Clear_Line(1);
                        Write_Str_LCD(0x80, "Error!");
                        Beep(2);
												error_count++;//增加一次错误
												if(error_count >= 3) {
                            locked = 1;
                            lock_timer = system_timer;
                            Clear_Line(1);
                            Write_Str_LCD(0x80, "Locked 5s...");
                            Beep(2);
														lock_seconds = 5;
                            delayms(1000);
												}
                    }
                    Reset_System();
                    delayms(1000);
                    Write_Str_LCD(0x80, "Enter Password:");
                    
                } else if(admin_mode == 1) {
                    // 此处判断语句开始执行管理员模式
                    if(strcmp(input_password, default_password) == 0) {
                        admin_mode = 2;
                        password_step = 1;
                        input_index = 0;
                        memset(input_password, 0, sizeof(input_password));
                        Clear_Line(1);
                        Clear_Line(2);
                        Write_Str_LCD(0x80, "Set New Password:");
                    } else {
                        Clear_Line(1);
                        Write_Str_LCD(0x80, "Admin Auth Fail!");
                        Beep(2);
                        Reset_System();
                        delayms(2000);
                        Write_Str_LCD(0x80, "Enter Password:");
                    }
                } else if(admin_mode == 2) {
                    if(password_step == 1) {
                        strcpy(new_password, input_password);
                        password_step = 2;
                        input_index = 0;
                        memset(input_password, 0, sizeof(input_password));
                        Clear_Line(2);
                        Write_Str_LCD(0xC0, "Confirm Password:");
                    } else if(password_step == 2) {
                        if(strcmp(input_password, new_password) == 0) {
                            
                            strcpy(default_password, new_password);
                            Clear_Line(1);
                            Clear_Line(2);
                            Write_Str_LCD(0x80, "Password Updated!");
                            Beep(1);
                        } else {
                            Clear_Line(1);
                            Write_Str_LCD(0x80, "Not Match! Fail!");
                            Beep(2);
                        }
                        Reset_System();
                        delayms(2000);
                        Write_Str_LCD(0x80, "Enter Password:");
                    }
                }
                
            } else if(key == 13) { //清除密码
                input_index = 0;
                memset(input_password, 0, sizeof(input_password));
                Clear_Line(2);
                
            } else if(key == 15) { //进入管理员模式(*)
                if(admin_mode == 0) {
                    admin_mode = 1;
                    input_index = 0;
                    memset(input_password, 0, sizeof(input_password));
                    Clear_Line(1);
                    Clear_Line(2);
                    Write_Str_LCD(0x80, "Admin Mode:");
                    Write_Str_LCD(0xC0, "Enter Password:");
                }
            }
        }
    }
}
//void main() {
//    uchar key;
//    Init_LCD();
//    Write_Str_LCD(0x80, "Enter Password:"); //

//    while(1) {
//        key = Get_Key();
//        if(key != 0xFF) {
//            if(key < 10) { // 
//                if(input_index < 6) {
//                    input_password[input_index++] = key + '0';
//                    Write_Char_LCD(0xC0 + input_index, '*'); // 
//                }
//            } else if(key == 12) { //确认键
//                input_password[input_index] = '\0';
//                if(strcmp(input_password, default_password) == 0) {
//                    Clear_Line(1);
//                    Write_Str_LCD(0x80, "Welcome!");
//                    Beep(1); // 
//                } else {
//                    Clear_Line(1);
//                    Write_Str_LCD(0x80, "Error!");
//                    Beep(2); // 
//                }
//                // 
//                input_index = 0;
//                memset(input_password, 0, sizeof(input_password));
//                delayms(2000);
//                Clear_Line(1);
//                Clear_Line(2);
//                Write_Str_LCD(0x80, "Enter Password:");
//            } else if(key == 13) { //清除键
//                input_index = 0;
//                memset(input_password, 0, sizeof(input_password));
//                Clear_Line(2);
//            }
//        }
//    }
//}