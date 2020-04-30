//
// Created by wang on 2020/4/30.
//

#ifndef FTP_COLOR_H
#define FTP_COLOR_H

const int F_BLACK = 0x01;	// 000001
const int F_RED = 0x02;	// 000010
const int F_GREEN = 0x03;	// 000011
const int F_YELLOW = 0x04;	// 000100
const int F_BLUE = 0x05;	// 000101
const int F_PURPLE = 0x06;	// 000110
const int F_WHITE = 0x07;	// 000111

const int B_BLACK = 0x08;	// 001000
const int B_RED = 0x10;		// 010000
const int B_GREEN = 0x18;	// 011000
const int B_BROWN = 0x80;	// 100000
const int B_BLUE = 0x88;	// 101000
const int B_WHITE = 0x90;	// 110000
//缺一个111000，但就这些了

bool setColor(int color)
{
    // color是一个前景色于一个后景色的位或结果
    bool ret = true;
    int fore = color%8;	//取color的后3位
    int back = (color/8)*8;	//将color的后3位清空 (即取前3位)
    switch (fore)
    {
        case F_BLACK:std::cout<<"\033[30m";break;
        case F_RED:std::cout<<"\033[31m";break;
        case F_GREEN:std::cout<<"\033[32m";break;
        case F_YELLOW:std::cout<<"\033[33m";break;
        case F_BLUE:std::cout<<"\033[34m";break;
        case F_PURPLE:std::cout<<"\033[35m";break;
        case F_WHITE:std::cout<<"\033[37m";break;
        default:ret = false;
    }
    switch (back)
    {
        case B_BLACK:std::cout<<"\033[40m";break;
        case B_RED:std::cout<<"\033[41m";break;
        case B_GREEN:std::cout<<"\033[42m";break;
        case B_BROWN:std::cout<<"\033[43m";break;
        case B_BLUE:std::cout<<"\033[44m";break;
        case B_WHITE:std::cout<<"\033[47m";break;
        default:ret = false;
    }
}

void resetFColor()
{std::cout<<"\033[39m";} // 重置前景色

void resetBColor()
{std::cout<<"\033[49m";} // 重置背景色

#endif //FTP_COLOR_H
