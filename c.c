/*

main(){int W=15,S=W*W,*m=calloc(S,4),z[2]={0},l=3,c=76,C,i,*p=m;srand(m);f:for(
++l;m[i=rand()%S];);for(m[i]--;C-80;_sleep(99)){for(i=system("cls"),*p=l;i<S;++
i%W||puts("|"))printf(m[i]>0&&m[i]--?"[]":m[i]?"00":"  ");if(kbhit())C=getch()&
95,C-1>>2^18||(c^C)&3^2&&(c=C);p=z+c%2,*p+=~-c&2,*p=(--*p+W)%W;p=m+*z+z[1]*W;if
(*p<0)goto f;if(*p)break;}}
// 351
*/

main(){int W=15,S=W*W,*m=calloc(S,4),z[2]={0},l=3,c=76,i,*p=m;srand(m);f:for(++
l;m[i=rand()%S];);for(m[i]--;c-80;_sleep(S)){for(i=system("cls"),*p=l;i<S;++i%
W||puts("|"))printf(m[i]>0&&m[i]--?"[]":m[i]?"00":"  ");kbhit()&&(c=getch()&95)
;p=z+c%2,*p+=~-c&2,*p=--*p%W,p=m+*z+z[1]*W;if(*p<0)goto f;}}
// 297