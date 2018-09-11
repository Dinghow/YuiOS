#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

void welcomeAnimation(){
	int t=500;
	printf("         t# ##.:E###K.        ,K##################G.             #f          \n");
	milli_delay(t);
	printf("         W#        .W##W.f###.                   ,f######:###  LD  #W        \n");
	milli_delay(t);
	printf("         K#           t####G                                      ##         \n");
	milli_delay(t);
	printf("         G#  :K#t                                        .E       W#         \n");
	milli_delay(t);
	printf("         i#       j;                                     #        ##         \n");
	milli_delay(t);
	printf("         ;#        .                                     K        ##         \n");
	milli_delay(t);
	printf("         .#       ,i                                     #       #D          \n");
	milli_delay(t);
	printf("          #                                               K      #i          \n");
	milli_delay(t);
	printf("          #       E                                         iE    #.         \n");
	milli_delay(t);
	printf("           #     f                                               :#          \n");
	milli_delay(t);
	printf("            #                                                     L#         \n");
	milli_delay(t);
	printf("          #                                         WD#          #E          \n");
	milli_delay(t);
	printf("         i#         #Wi                           #####K#          :#        \n");
	milli_delay(t);
	printf("         G#            ####E,                    #########           #       \n");
	milli_delay(t);
	printf("         #,           ###i                       #########          #L       \n");
	milli_delay(t);
	printf("         #.         t.                            #######           WK       \n");
	milli_delay(t);
	printf("         #                                         ,W#Wt         ##W##GLt;.  \n");
	milli_delay(t);
	printf("    W########WK                                                     i#       \n");
	milli_delay(t);
	printf("         #                                                       #,.,#       \n");
	milli_delay(t);
	printf("         #                             :i                          .K##Wj.   \n");
	milli_delay(t);
	printf("        .###Wj.                #       iK       #                   #W    tK \n");
	milli_delay(t);
	printf("    L##Wj#D                    #.      ##       #                   #,       \n");
	milli_delay(t);
	printf("         f#                      ####i#t######E                    #         \n");
	milli_delay(t);
	printf("          #j                                                      #i         \n");
	milli_delay(t);
	printf("            .###f                                               ##G          \n");
	milli_delay(t);
	printf("               W##                                            :G#            \n");
	milli_delay(t);
	printf("                  ###iK#######################################               \n");
	milli_delay(t);
	printf("\n");
	milli_delay(t);
	printf("\n");
	milli_delay(t);
	printf("\n");
	milli_delay(t);
	printf("\n");
}

PUBLIC void welcome(){    
	printf(" ___________________________________________________________________________ \n");
	printf("|                                                                           |\n");
	printf("|==================================YuiOS====================================|\n");
	printf("|                                                                           |\n");
	printf("|                             1652739 Yakang Li                             |\n");
	printf("|                             1652749 Jinrong Huang                         |\n");
	printf("|                             1652741 Dinghow Yang                          |\n");
	printf("|                                                                           |\n");
	printf("|===========================================================================|\n");
	printf("|___________________________________________________________________________|\n");
	printf("\n");
	printf("\n");
	printf("\n");       
	//printf("help\n");
}
