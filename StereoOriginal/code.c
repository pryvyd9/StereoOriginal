#include<stdio.h>
//#include<memory>
//#include<conio.h>
//#include<dos.h>
//#include<iostream>
//#include<stdlib.h>
//#include<math.h>
#include<graphics.h>
//#include "svga16.h"

#define Del   0x53
#define Home  0x47
#define End   0x4f
#define Up    0x48
#define Down  0x50
#define Left  0x4b
#define Right 0x4d
#define Esc   0x1b
#define AltX  45
#define F10   68
#define F2    60
#define F3    61
#define PI    3.1415296
#define NVEC  2500


int huge detectvga16();
void wf(void);
void rf(void);
void clgr(void);
void cldev(void);
void veki(void);
void veks(void);
int fpg = 0;
void pages();
void mas(int n, int n1, float x0, float y0, float z0);
void s_vek(float buf[], int in, int ik);
void s_pic(float x, float y, float z, float* xl, float* xp, float* ys);
static float t[18];
void marker(float* t);
void pic_m(float* t);
void upr_pic(void);
void povor(float df, int i1, int i2);
float c0[] = { 0,0,0 };
float f = PI / 12;
void povora(float f, int* fl, int* f1, int* f2, int i1, int i2);
float x1, y1, z1, x2, y2, z2, x3, y3, z3,
x, y, z, t1, t2, t3, t4, t5, t6;
int kt = 12;
void pero(void);
void rabt(float xn, float yn);
float dd(float xn, float yn, float zn, float xk, float yk, float zk);
void ingr(void);
int graphdriver = DETECT, graphmode, svcolor;

//struct REGPACK r;
struct REGPACK
{

	unsigned r_ax, r_bx, r_cx, r_dx;

	unsigned r_bp, r_si, r_di, r_ds, r_es, r_flag;

} r;
void menu(void);
int chw, chh;
char name[] = "Noname.s", sxyz[50];
int msinit(int Xlo, int Xhi, int Ylo, int Yhi);
void m_kur(int X, int Y);
int msread(int* pX, int* pY, int* pbuttons);
void textxy(int X, int Y, int color, char* str);
void setbar(int x1, int y1, int x2, int y2, int color);
void clearm(int x1, int y1, int x2, int y2);
struct Pmenu { char p[20]; };
void drawm(int x1, int y1, int x2, int y2,
	int hy, int hx, int nv, struct Pmenu Vm[]);
void choice(int i0, int j, struct Pmenu* Vm,
	int x1, int hx, int yi, int hy);
char* bm, * bi, * bi0;
void wtext(int x, int yi, int col, char* str);
int getstr(int x, int y, int x2, int i, char* str);
int getstr1(int x1, int y1, int x2, int y2,
	int hx, int yi, char* str, char* Pch);
void getstr2(int x1, int y1, int x2, int y2,
	int hx, int yi, char* t1, char* t2, char* t3,
	char* str1, char* str2, char* str3);

int kh = 32, hc = 8, hr = 8, l = 8, n = 10;
int a = 32, rx, ry, rz = 1024;
int cx, cy, cz = 0, c1, c2, c3;
int  F_p = 0, F_wr = 0;
float x0 = 0, y0 = 0, z0 = 0;

FILE* fp;
unsigned j = 0;
float* v, * vrab, * vb;
static	float t[18];

main()
{
	
	char c;
	int  f_cl = 0;
	unsigned i, size = 18000;
	v = (float*)farcalloc(NVEC * 6, sizeof(float));
	vrab = (float*)farcalloc(NVEC * 6, sizeof(float));
	vb = (float*)farcalloc((NVEC / 2) * 6, sizeof(float));
	bm = (char*)farmalloc(3 * size);
	bi = (char*)farmalloc(3 * size);
	bi0 = (char*)farmalloc(size);
	/*	printf("%lu  ",farcoreleft());
		getch();*/
	ingr();
	settextjustify(1, 1);
	settextstyle(2, 0, 6);
	chw = textwidth("A");
	chh = textheight("A");
	moveto(5, 15);
	if (msinit(0, getmaxx(), 0, getmaxy()) == 0) { printf(" not M ? "); exit(1); }
	while (1)
	{
		c = getch();
		switch (c)
		{
		case 'z':   F_p = 0; x1 = x0; y1 = y0; z1 = z0; break;
		case 'd':	  if (fpg == 1) setvisualpage(1);
					  else setvisualpage(0);
			s_vek(t, 0, 18); F_p = 0; break;
		case 'x':   F_p = 1;  x3 = x0; y3 = y0; z3 = z0; goto vvv;
		case '[':   kh = 1; break;
		case ']':   kh = 8; break;
		case '5':   x0 = 0; y0 = 0; z0 = 0; break;
		case '4':   x0 = x0 - kh; break;
		case '6':   x0 = x0 + kh; break;
		case '2':   y0 = y0 - kh; break;
		case '8':   y0 = y0 + kh; break;
		case '1':   z0 = z0 + kh; break;
		case '9':   z0 = z0 - kh; break;
		case '7':   l = l / 2;  break;
		case '3':   l = l + 2;  break;
		case 'i':   veki(); break;
		case 's':   veks(); break;
		case 'o':   j = j - 6; s_vek(v, j, j + 6); break;
		case 'v':   s_vek(v, 0, j); s_vek(v, 0, j); break;
		case  0:   c = getch();
			switch (c)
			{
			case   AltX: clgr();
			case   F10: menu(); cldev();
				break;
			case   F2: wf(); break;
			case   Del: cleardevice(); x0 = y0 = z0 = 0; j = 0;
				s_vek(t, 0, 18); break;
			case  Home: for (i = 0; i <= j - 3; i = i + 3)
			{
				v[i] = v[i] - x0;
				v[i + 1] = v[i + 1] - y0;
				v[i + 2] = v[i + 2] - z0;
			}
						cx = cx + x0; cy = cy - y0; cz = cz + z0;
						x = x0; y = y0; z = z0;
						x0 = y0 = z0 = 0; pic_m(t);
						upr_pic();
						break;
			} break;
		case '+': 	upr_pic(); 	break;
		case 'c':   f_cl = 1; break;
		case 'a':   cldev(); f_cl = 0;	break;
		}
		/*printf("x %-5d y %-5d z %-5d\r",(int)x0,(int)y0,(int)z0);*/
		setbar(5 + chw, 15 - chh, 5 + 6 * chw, 15 + chh, 0);
		setbar(5 + 7 * chw, 15 - chh, 5 + 15 * chw, 15 + chh, 0);
		setbar(5 + 15 * chw, 15 - chh, 5 + 23 * chw, 15 + chh, 0);
		sprintf_s(sxyz, "x:%-4dy:%-4dz:%-4d", (int)x0, (int)y0, (int)z0);
		wtext(5, 15, 7, sxyz);
		if (F_p == 1)
		{
			if (f_cl == 0) cleardevice();
			else { s_vek(v, 0, j); }
			/*printf("x %-5d y %-5d z %-5d\r",(int)x0,(int)y0,(int)z0);*/
			setbar(5 + chw, 15 - chh, 5 + 6 * chw, 15 + chh, 0);
			setbar(5 + 7 * chw, 15 - chh, 5 + 15 * chw, 15 + chh, 0);
			setbar(5 + 15 * chw, 15 - chh, 5 + 23 * chw, 15 + chh, 0);
			sprintf_s(sxyz, "x:%-4dy:%-4dz:%-4d", (int)x0, (int)y0, (int)z0);
			wtext(5, 15, 7, sxyz);
			x2 = x0; y2 = y0; z2 = z0;
			pero();
			s_vek(v, 0, j);
			pages();
		}
		else marker(t);
	vvv:;
	}
};

void upr_pic(void)
{
	unsigned i, ii = 0;
	int key, Ok = 1, f_cl = 0,
		fl_b = 0, fl_x = 0, fl_y = 0, fl_z = 0;
	while (Ok)
	{
		key = getch();
		if (f_cl == 0) cleardevice();
		else  s_vek(v, 0, j);
		switch (key)
		{
		case '.': 	Ok = 0;
		case 'g':  	fl_b = 1; break;
		case 'v':   fl_b = 0; for (i = 0; i <= ii - 1; i++) v[i] = vb[i];
			j = ii; break;
		case '1':  	cx = cx - hc; break;
		case '3':  	cx = cx + hc; break;
		case '5':  	cy = cy - hc; break;
		case '2':  	cy = cy + hc; break;
		case '6':  	cz = cz - hc; break;
		case '4':  	cz = cz + hc; break;
		case '7':  	rx = rx - hr; break;
		case '9':	  rx = rx + hr; break;
		case '/':	  ry = ry - hr; break;
		case '8':	  ry = ry + hr; break;
		case '-':	  rz = rz - hr; break;
		case '*':	  rz = rz + hr; break;
		case 'a':	 	cldev(); s_vek(v, 0, j);
			s_vek(t, 0, 18); f_cl = 0;
			break;
		case 'c':	  f_cl = 1; break;
		case '[':   povora(f, &fl_y, &fl_x, &fl_z, 0, 2);  break;
		case ']':   povora(-f, &fl_y, &fl_x, &fl_z, 0, 2); break;
		case 'o':   povora(f, &fl_x, &fl_y, &fl_z, 1, 2);  break;
		case 'p':   povora(-f, &fl_x, &fl_y, &fl_z, 1, 2); break;
		case '\'':  povora(f, &fl_z, &fl_x, &fl_y, 0, 1);  break;
		case ';':   povora(-f, &fl_z, &fl_x, &fl_y, 0, 1); break;
		case 'l':   if (n == 1) { s_vek(v, 0, j); s_vek(v, 0, j); break; }
					mas(n, n - 1, x0, y0, z0);
					n = n - 1;
					break;
		case 'b':	  mas(n, n + 1, -x0, -y0, -z0);
			n = n + 1; break;
		case  0:   key = getch();
			switch (key)
			{
			case  End: cx = c1; cy = c2; cz = c3;
				for (i = 0; i <= j - 3; i = i + 3)
				{
					v[i] = v[i] + x;
					v[i + 1] = v[i + 1] + y;
					v[i + 2] = v[i + 2] + z;
				}
				x0 = x; y0 = y, z0 = z; pic_m(t);
				Ok = 0; break;
			}
			break;
		}
		if (fl_b == 1) for (i = 0; i <= j - 1; i++, ii++)	vb[ii] = v[i];
		s_vek(v, 0, j);
		s_vek(t, 0, 18);
		if (Ok == 0)
		{
			if (fpg == 1) { setvisualpage(1); return; }
			else { setvisualpage(0);  return; }
		}
		pages();
	}
};




void ingr(void)
{
	installuserdriver("Svga16", detectvga16);
	initgraph(&graphdriver, &graphmode, "");
	svcolor = getcolor();
	setwritemode(1);
	gotoxy(2, 1);
	cx = 1 + getmaxx() / 2; cy = 1 + getmaxy() / 2;
	c1 = cx; c2 = cy; c3 = cz;
	rx = cx; ry = 0;
	s_vek(t, 0, 18);
	marker(t);
};

void clgr(void)
{
	setcolor(svcolor);
	farfree(v); farfree(vrab);
	farfree(vb);
	farfree(bi); farfree(bm);
	farfree(bi0);
	closegraph();

};

void cldev(void)
{
	cleardevice();
	s_vek(v, 0, j);
	s_vek(t, 0, 18);
};

int huge detectvga16()
{
	int Vid = 4;
	return Vid;
};

void marker(float* t)
{
	s_vek(t, 0, 18);
	pic_m(t);
	s_vek(t, 0, 18);

};

void pic_m(float* t)
{
	t[0] = x0 - l; t[3] = x0 + l; t[1] = t[4] = y0; t[2] = t[5] = z0;
	t[8] = z0 + l; t[11] = z0 - l; t[6] = t[9] = x0; t[7] = t[10] = y0;
	t[13] = y0 + l; t[16] = y0 - l; t[12] = t[15] = x0; t[14] = t[17] = z0;
}



void veki(void)
{
	v[j] = x0;
	v[j + 1] = y0;
	v[j + 2] = z0;
};

void veks(void)
{
	v[j + 3] = x0;
	v[j + 4] = y0;
	v[j + 5] = z0;
	s_vek(v, j, j + 6);
	j = j + 6;
	veki();
};

/* бвҐаҐ®®ЇҐа в®а Ї®бв®п­­®Ј® а Єгаб  */
void s_pic(float x, float y, float z,
	float* xl, float* xp, float* ys)
{
	float  d;
	d = rz - cz - z;
	*xl = ((x + cx) * rz - (z + cz) * (rx - a)) / d + 0.5;
	*xp = ((x + cx) * rz - (z + cz) * (rx + a)) / d + 0.5;
	*ys = ((cy - y) * rz - ry * (z + cz)) / d + 0.5;
};

/* бвҐаҐ®ўҐЄв®а */
void  s_vek(float buf[], int in, int ik)
{
	int x1, y1, x2, y2, x3, y3, x4, y4, i;
	float xl, xp, ys;
	for (i = in; i <= ik - 6; i = i + 6)
	{
		s_pic(buf[i], buf[i + 1], buf[i + 2], &xl, &xp, &ys);
		x1 = xl; y1 = ys; x3 = xp; y3 = ys;
		s_pic(buf[i + 3], buf[i + 4], buf[i + 5], &xl, &xp, &ys);
		x2 = xl; y2 = ys; x4 = xp; y4 = ys;
		setcolor(3); line(x1, y1, x2, y2);
		setcolor(4); line(x3, y3, x4, y4);
	}
};

void mas(int n, int n1,
	float x0, float y0, float z0)
{
	int i;
	/*	if(Fcled==0) s_vek(v,0,j); */
	for (i = 0; i <= j - 3; i = i + 3)
	{
		v[i] = (v[i] * n1 + x0) / n;
		v[i + 1] = (v[i + 1] * n1 + y0) / n;
		v[i + 2] = (v[i + 2] * n1 + z0) / n;
	}
};


void pages()
{
	if (fpg == 1)
	{
		setvisualpage(1);
		setactivepage(0);
		fpg = 0;
	}
	else
	{
		setvisualpage(0);
		setactivepage(1);
		fpg = 1;
	}
	delay(150);
}


/* Ї®ў®а®в */
void povor(float df, int i1, int i2)
{
	int i;
	float c, s;
	c = cos(2 * PI - df); s = -sin(2 * PI - df);
	for (i = 0; i <= j - 3; i = i + 3)
	{
		v[i + i1] = vrab[i + i1] * c - vrab[i + i2] * s;
		v[i + i2] = vrab[i + i1] * s + vrab[i + i2] * c;
	}
};

void povora(float f, int* fl, int* f1,
	int* f2, int i1, int i2)
{
	static float df;
	int i;
	if (*fl == 1)
	{
		df = df + f;
		povor(df, i1, i2);
	}
	else
	{
		for (i = 0; i <= j - 1; i++) vrab[i] = v[i];
		povor(f, i1, i2); df = 0;
		*fl = 1; *f1 = 0; *f2 = 0;
	}
}

void pero(void)
{
	int i;
	float d0, d1, d2, d3,
		rd, x4, y4, z4,
		xn, yn;
	j = 0;
	d0 = dd(x1, y1, z1, x3, y3, z3);
	if (d0 == 0.0)  printf(" d0=0 ");
	t1 = (x3 - x1) / d0;
	t2 = (y3 - y1) / d0;
	t3 = (z3 - z1) / d0;
	d1 = dd(x1, y1, z1, x2, y2, z2);
	d2 = dd(x2, y2, z2, x3, y3, z3);
	rd = (d1 * d1 + d0 * d0 - d2 * d2) / (2 * d0);
	d3 = d1 * d1 - rd * rd;
	if (d3 <= 0.0)
	{
		v[0] = x1; v[1] = y1; v[2] = z1;
		v[3] = x3; v[4] = y3; v[5] = z3;
		j = j + 6; return;
	}
	d3 = sqrt(d1 * d1 - rd * rd);
	x4 = (rd * (x3 - x1) + d0 * x1) / d0;
	y4 = (rd * (y3 - y1) + d0 * y1) / d0;
	z4 = (rd * (z3 - z1) + d0 * z1) / d0;
	t4 = (x4 - x2) / d3;
	t5 = (y4 - y2) / d3;
	t6 = (z4 - z2) / d3;
	xn = t1 * (x1 - x2) + t2 * (y1 - y2) + t3 * (z1 - z2);
	yn = t4 * (x1 - x2) + t5 * (y1 - y2) + t6 * (z1 - z2);
	rabt(xn, yn);
	xn = t1 * (x3 - x2) + t2 * (y3 - y2) + t3 * (z3 - z2);
	yn = t4 * (x3 - x2) + t5 * (y3 - y2) + t6 * (z3 - z2);
	rabt(xn, yn);
}


float dd(float xn, float yn, float zn,
	float xk, float yk, float zk)
{
	float x, y, z;
	x = xk - xn; y = yk - yn; z = zk - zn;
	return(sqrt(x * x + y * y + z * z));
}


void rabt(float xn, float yn)
{
	float xx[101], yy[101], xi, yi;
	int i;
	if (abs(xn) >= abs(yn))
		for (i = 0, xi = xn / kt, yi = yn / (kt * kt); i <= kt; i = i++)
		{
			xx[i] = i * xi; yy[i] = yi * i * i;
		}
	else
	{
		if (xn >= 0)
			for (i = 0, yi = yn / kt, xi = xn * xn / kt; i <= kt; i = i++)
			{
				yy[i] = i * yi; xx[i] = sqrt(xi * i);
			}
		else
			for (i = 0, yi = yn / kt, xi = xn * xn / kt; i <= kt; i = i++)
			{
				yy[i] = i * yi; xx[i] = -sqrt(xi * i);
			}
	}
	v[j] = t1 * xx[0] + t4 * yy[0] + x2;
	v[j + 1] = t2 * xx[0] + t5 * yy[0] + y2;
	v[j + 2] = t3 * xx[0] + t6 * yy[0] + z2;
	for (j = j + 3, i = 1; i <= kt - 1; i = i++, j = j + 6)
	{
		v[j] = v[j + 3] = t1 * xx[i] + t4 * yy[i] + x2;
		v[j + 1] = v[j + 4] = t2 * xx[i] + t5 * yy[i] + y2;
		v[j + 2] = v[j + 5] = t3 * xx[i] + t6 * yy[i] + z2;
	}
	v[j] = t1 * xx[kt] + t4 * yy[kt] + x2;
	v[j + 1] = t2 * xx[kt] + t5 * yy[kt] + y2;
	v[j + 2] = t3 * xx[kt] + t6 * yy[kt] + z2;
	j = j + 3;
};

void menu(void)
{
	int buttons, X, Y, ch,
		hx = 35, hy = 24, yg = 30, nv1 = 5, nv2 = 7, nv3 = 3, ng = 5,
		x = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0, xl, xp, yi,
		xv1, yv1, xv2, yv2, xv3, yv3, jn, j0 = 0,
		i, i0 = 100, k, y = 45,
		max_x, max_y;
	struct Pmenu* Vm;
	Vm = 0;
	struct Pmenu Pm[5] = { {"-"},{"File"},{"System"},{"Tools"},{"Help"} };
	struct Pmenu Vm1[5] = { {"New"},{"Open      "},{"Close     F2"},{"Save As"},{"Exit"} };
	struct Pmenu Vm2[7] = { {"Cx Cy Cz       "},{"XYZ step       "},{"Eyebasis"},
	{"Rx Ry Rz        "},{"Rakurs step    "},{"Set cursor   "},{"Cursor step"} };
	struct Pmenu Vm3[3] = { {"Rotation"},{"Interpolation"},{"Scale"} };
	max_x = getmaxx();
	max_y = getmaxy();
	getimage(1, 1, max_x - 1, y, bi);
	getimage(1, 1, max_x - 1, y, bi0);
	setbar(1, 1, max_x - 1, 20, 1);
	textxy((max_x - 1) / 2, 10, 15, "Stereo");
	setbar(1, 21, max_x - 1, y, 15);
	for (i = 0, k = 1; i <= ng - 1; i++, k += 2)
		textxy(k * hx, yg, 0, Pm[i].p);
	textxy(hx, yg + 1, 0, "-");
	X = max_x; Y = max_y;
	y1 = y + 1;
	yv1 = y1 + hy * (nv1 + 1); xv1 = hx * 4 + 15;
	yv2 = y1 + hy * (nv2 + 1); xv2 = hx * 6 - 5;
	yv3 = y1 + hy * (nv3 + 1); xv3 = hx * 6 - 5;
	while (1)
	{
		msread(&X, &Y, &buttons);
		if (buttons && X < 350 && Y < y)
		{
			i = X / (2 * hx);
			switch (i0)
			{
			case 0:  break;
			case 1:  setbar(xl, 22, xp, 44, 15);
				textxy(x, yg, 0, Pm[i0].p);
				clearm(x1, y1, x2, y2);
				break;
			case 2:  setbar(xl, 22, xp, 44, 15);
				textxy(x, yg, 0, Pm[i0].p);
				clearm(x1, y1, x2, y2);
				break;
			case 3:  setbar(xl, 22, xp, 44, 15);
				textxy(x, yg, 0, Pm[i0].p);
				clearm(x1, y1, x2, y2);
				break;
			case 4:  setbar(xl, 22, xp, 44, 15);
				textxy(x, yg, 0, Pm[i0].p);
				break;
			}
			x = hx * (2 * i + 1);
			x1 = x - hx;
			xl = x - textwidth(Pm[i].p) / 2 - 6;
			xp = x + textwidth(Pm[i].p) / 2 + 6;
			if (i == i0) i0 = 100;
			else
			{
				switch (i)
				{
				case 0: getimage(1, 1, max_x - 1, y, bm);
					putimage(1, 1, bm, XOR_PUT);
					putimage(1, 1, bi0, XOR_PUT);
					return;
				case 1: setbar(xl, 22, xp, 44, 1);
					textxy(x, yg, 15, "File");
					x1 = x1 + 10;
					x2 = x1 + xv1; y2 = yv1;
					getimage(x1, y1, x2, y2, bi);
					drawm(x1, y1, x2, y2, hy / 2, 20, nv1, Vm1);
					jn = 0; yi = (hy / 2) + 45; Vm = Vm1;
					break;
				case 2:  setbar(xl, 22, xp, 44, 1);
					textxy(x, yg, 15, "System");
					x2 = x1 + xv2; y2 = yv2;
					getimage(x1, y1, x2, y2, bi);
					drawm(x1, y1, x2, y2, hy / 2, 20, nv2, Vm2);
					jn = 0; yi = (hy / 2) + 45; Vm = Vm2;
					break;
				case 3:  setbar(xl, 22, xp, 44, 1);
					textxy(x, yg, 15, "Tools");
					x2 = x1 + xv3; y2 = yv3;
					getimage(x1, y1, x2, y2, bi);
					drawm(x1, y1, x2, y2, hy / 2, 20, nv3, Vm3);
					jn = 0; yi = (hy / 2) + 45; Vm = Vm3;
					break;
				case 4:  setbar(xl, 22, xp, 44, 1);
					textxy(x, yg, 15, "Help");
					break;
				}
				i0 = i;
			}
		}
		else
		{
			if (buttons && X<x2 && X>x1 && Y > y1 + 5 && Y < y2 - hy)
			{
				if (j0 != -1)
				{
					setbar(x1 + 8, yi - hy / 2 + 2, x2 - 8, yi + hy / 2, 15);
					settextjustify(0, 1);
					textxy(x1 + 20, yi, 0, Vm[jn].p);
				}
				jn = (Y - 45) / hy;
				yi = (hy / 2) * (2 * jn + 1) + 45;
				if (jn == j0)
				{
					setbar(xl, 22, xp, 44, 15);
					settextjustify(1, 1);
					textxy(x, yg, 0, Pm[i0].p);
					clearm(x1, y1, x2, y2);
					choice(i0, jn, Vm, x1, hx, yi, hy);
					x1 = 0; i0 = 100; j0 = 0;
					x1 = 0; x2 = 0;
				}
				else
				{
					setbar(x1 + 8, yi - hy / 2 + 3, x2 - 8, yi + hy / 2, 1);
					settextjustify(0, 1);
					textxy(x1 + 20, yi, 15, Vm[jn].p);
					settextjustify(1, 1);
					j0 = jn;
				}
			}
		}
		if (buttons);
		else
			m_kur(X, Y);


	}
};


void drawm(int x1, int y1, int x2, int y2,
	int hy, int hx, int nv, struct Pmenu Vm[])
{
	int j, k;
	settextjustify(0, 1);
	setbar(x1, y1, x2, y2, 15);
	setcolor(7);
	rectangle(x1 + 5, y1, x2 - 5, y2 - 5);
	setbar(x1 + 8, y1 + 2, x2 - 8, y1 + 2 * hy - 1, 1);
	textxy(x1 + hx, 1 * hy + y1, 15, Vm[0].p);
	for (j = 1, k = 3; j <= nv - 1; j++, k += 2)
		textxy(x1 + hx, k * hy + y1, 0, Vm[j].p);
	settextjustify(1, 1);
}

void choice(int i0, int jn, struct Pmenu* Vm,
	int x1, int hx, int yi, int hy)
{
	char ch, str1[20], str2[20], str3[20];
	char str[20];
	int y1, x2, y2, i;
	y1 = yi - hy / 2;
	x2 = x1 + 12 * hx; y2 = yi + 2 * hy + hy / 2;
	yi = yi + hy;
	switch (i0)
	{
	case 1: switch (jn)
	{
	case 0:   return;
	case 1:   if (getstr1(x1, y1, x2, y2, hx, yi, name, "File Name") == 27)return;
		getimage(1, 1, 799, 45, bm);
		cleardevice();
		rf();
		s_vek(v, 0, j);
		x0 = y0 = z0 = 0; s_vek(t, 0, 18);
		getimage(1, 1, 799, 45, bi0);
		putimage(1, 1, bm, COPY_PUT);
		return;
	case 2:   wf();
		return;
	case 3:   if (getstr1(x1, y1, x2, y2, hx, yi, name, "New Name") == 27)return;
		wf();
		return;
	case 4:   clgr();

	} break;
	case 2: switch (jn)
	{
	case 0:   sprintf_s(str1, "%d", cx);
		sprintf_s(str2, "%d", cy);
		sprintf_s(str3, "%d", cz);
		getstr2(x1, y1 + hy, x2, y2 + hy, hx, yi + hy,
			"Cx", "Cy", "Cz", str1, str2, str3);
		sscanf_s(str1, "%d", &cx);
		sscanf_s(str2, "%d", &cy);
		sscanf_s(str3, "%d", &cz);
		return;
	case 1:   sprintf_s(str, "%d", hc);
		getstr1(x1, y1 + hy, x1 + 6 * hx, y2 + hy, hx, yi + hy, str, "XYZ Step");
		sscanf_s(str, "%d", &hc);
		return;
	case 2:   sprintf_s(str, "%d", a);
		getstr1(x1, y1 + hy, x1 + 6 * hx, y2 + hy, hx, yi + hy, str, "Eyebasis");
		sscanf_s(str, "%d", &a);
		return;
	case 3:   sprintf_s(str1, "%d", rx);
		sprintf_s(str2, "%d", ry);
		sprintf_s(str3, "%d", rz);
		getstr2(x1, y1 + hy, x2, y2 + hy, hx, yi + hy,
			"Rx", "Ry", "Rz", str1, str2, str3);
		sscanf_s(str1, "%d", &rx);
		sscanf_s(str2, "%d", &ry);
		sscanf_s(str3, "%d", &rz);
		return;
	case 4:    sprintf_s(str, "%d", hr);
		getstr1(x1, y1 + hy, x1 + 6 * hx, y2 + hy, hx, yi + hy, str, "Rakurs Step");
		sscanf_s(str, "%d", &hr);
	case 5:    sprintf_s(str, "%d", l);
		getstr1(x1, y1 + hy, x1 + 6 * hx, y2 + hy, hx, yi + hy, str, "Cursor size");
		sscanf_s(str, "%d", &l);
		return;
	case 6:    sprintf_s(str, "%d", kh);
		getstr1(x1, y1 + hy, x1 + 6 * hx, y2 + hy, hx, yi + hy, str, "Cursor Step");
		sscanf_s(str, "%d", &kh);
		return;
	}
			break;
	case 3: switch (jn)
	{
	case 0:   sprintf_s(str, sizeof(str), "%f", f);
		getstr1(x1, y1 + hy, x1 + 6 * hx, y2 + hy, hx, yi + hy, str, "Angle");
		sscanf_s(str, "%f", &f);
		return;
	case 1:   sprintf_s(str, "%d", kt);
		getstr1(x1, y1 + hy, x1 + 6 * hx, y2 + hy, hx, yi + hy, str, "Number of point");
		sscanf_s(str, "%d", &kt);
		return;
	case 2:   sprintf_s(str, "%d", n);
		getstr1(x1, y1 + hy, x1 + 6 * hx, y2 + hy, hx, yi + hy, str, "Scale");
		sscanf_s(str, "%d", &n);
		return;
	}
			break;
	}
};

void wf(void)
{
	int i;
	fopen_s(name, fp, "w");
	rewind(fp);
	if (fp == 0) { textxy(700, 55, 7, "File problem"); return; }
	if (j == 0) { fclose(fp); return; }
	fprintf(fp, "%u\n", j);
	fprintf(fp, "%d %d %d %d %d %d %d\n", a, cx, cy, cz, rx, ry, rz);
	for (i = 0; i <= j - 6; i = i + 6)
		fprintf(fp, "%.0f %.0f %.0f %.0f %.0f %.0f\n",
			v[i], v[i + 1], v[i + 2], v[i + 3], v[i + 4], v[i + 5]);
	fclose(fp);
};

void rf(void)
{
	int i;
	fopen_s(name, fp, "r");
	rewind(fp);
	if (fp == 0) { textxy(700, 55, 7, "Input file problem"); return; }
	fscanf_s(fp, "%u", &j);
	fscanf_s(fp, "%d %d %d %d %d %d %d", &a, &cx, &cy, &cz, &rx, &ry, &rz);
	for (i = 0; i <= j - 6; i = i + 6)
		fscanf_s(fp, "%f %f %f %f %f %f", &v[i], &v[i + 1], &v[i + 2], &v[i + 3], &v[i + 4], &v[i + 5]);
	fclose(fp);
};

int getstr1(int x1, int y1, int x2, int y2,
	int hx, int yi, char* str, char* Pch)
{
	int x, i = 0;
	getimage(x1, y1, x2, y2, bi);
	setcolor(7);
	setbar(x1, y1, x2, y2, 15);
	rectangle(x1 + 5, y1 + 10, x2 - 5, y2 - 10);
	setbar(x1 + (x2 - x1) / 2 - strlen(Pch) * chw / 2, y1 + 10, x1 + chw / 2 + (x2 - x1) / 2 + strlen(Pch) / 2 * chw, y1 + 20, 15);
	textxy(x1 + (x2 - x1) / 2, y1 + 10, 1, Pch);
	settextjustify(0, 1);
	wtext(x1 + hx, yi, 0, str);
	if (getstr(x1 + hx, yi, x2 - 20, strlen(str), str) == 27) i = 27;
	clearm(x1, y1, x2, y2);
	settextjustify(1, 1);
	return i;
};

void  getstr2(int x1, int y1, int x2, int y2, int hx,
	int yi, char* t1, char* t2, char* t3,
	char* str1, char* str2, char* str3)
{
	int x, y, len, i;
	char str[20], ch;
	setcolor(7);
	y2 = y2 + 8 * chh + 5;
	x2 = x1 + 6 * hx;
	getimage(x1, y1, x2, y2, bi);
	x = x1 + 2 * hx + chw / 2;
	setbar(x1, y1, x2, y2, 15);
	rectangle(x1 + 5, y1 + 10, x2 - 5, y2 - 10);
	y = yi;
	for (i = 1; i <= 3; i++)
	{
		rectangle(x - chw, y - (chh / 2) - 3, x2 - hx, y + (chh / 2) + 6);
		y = y + 3 * chh;
	}

	settextjustify(0, 1);
	wtext(x, yi, 7, str1);
	wtext(x, yi + 3 * chh, 7, str2);
	wtext(x, yi + 6 * chh, 7, str3);
	textxy(x1 + hx, yi, 1, t1);
	textxy(x1 + hx, yi + 3 * chh, 1, t2);
	textxy(x1 + hx, yi + 6 * chh, 1, t3);
	textxy(x, yi + 9 * chh, 7, "Enter");
	y = yi;
	while (1)
	{
		if (y == yi)
		{
			wtext(x, y, 0, str1);
			if (getstr(x, y, x2 - hx - 10, strlen(str1), str1) == 27) break;
			setbar(x - chw / 3, y - chh / 2, x2 - hx - 3, y + chh / 2 + 3, 15);
			wtext(x, y, 7, str1);
		}
		if (y == yi + 3 * chh)
		{
			wtext(x, y, 0, str2);
			if (getstr(x, y, x2 - hx - 10, strlen(str2), str2) == 27) break;
			setbar(x - chw / 3, y - chh / 2, x2 - hx - 3, y + chh / 2 + 3, 15);
			wtext(x, y, 7, str2);
		}
		if (y == yi + 6 * chh)
		{
			wtext(x, y, 0, str3);
			if (getstr(x, y, x2 - hx - 10, strlen(str3), str3) == 27) break;
			setbar(x - chw / 3, y - chh / 2, x2 - hx - 3, y + chh / 2 + 3, 15);
			wtext(x, y, 7, str3);
			textxy(x, yi + 9 * chh, 0, "Enter");
			ch = getch();
			if (ch == '\r') break;
			if (ch == 9) { textxy(x, yi + 9 * chh, 7, "Enter"); y = yi; }
		}
		else
			y = y + 3 * chh;
	}
	settextjustify(1, 1);
	clearm(x1, y1, x2, y2);
};

void setbar(int x1, int y1, int x2, int y2, int color)
{
	setfillstyle(1, color);
	bar(x1, y1, x2, y2);
};

void clearm(int x1, int y1, int x2, int y2)
{

	getimage(x1, y1, x2, y2, bm);
	putimage(x1, y1, bm, XOR_PUT);
	putimage(x1, y1, bi, XOR_PUT);
};

/*void wxyz(int x,int y,int col)
{ char str[20]; */


void wtext(int x, int yi, int col, char* str)
{
	int i;
	char s2[2];
	s2[1] = '\0';
	for (i = 0; i <= strlen(str); i++)
	{
		s2[0] = str[i];
		textxy(x, yi, col, s2);
		x = x + chw;
	}
};

int getstr(int x, int y, int x2, int i, char* str)
{
	int j, k;
	char ch = 0, s2[2];
	s2[1] = '\0';
	while (1)
	{
		j = i * chw;
		textxy(x + j, y, 0, "_");
		textxy(x + j, y + 1, 0, "_");
		ch = getch();
		textxy(x + j, y, 15, "_");
		textxy(x + j, y + 1, 15, "_");
		if (ch == 27) break;
		if (ch == '\r' || ch == 9) break;
		if (ch == 8)
		{
			if (--i < 0) i = 0;
			k = i * chw;
			setbar(x + k - chw / 4, y - (chh / 2) - 1, x + k + chw, y + (chh / 2) + 4, 15);
		}
		else
		{
			if (ch != 0 & (x + j) < x2)
			{
				str[i] = s2[0] = ch;
				textxy(x + j, y, 0, s2);
				i++;
			}
			else
				ch = getch();
		}
	}
	str[i] = '\0';
	return ch;
};

void textxy(int X, int Y, int color, char* str)
{
	setwritemode(COPY_PUT);
	setcolor(color);
	outtextxy(X, Y, str);
	outtextxy(X + 1, Y, str);
	outtextxy(X, Y + 1, str);
	outtextxy(X + 1, Y + 1, str);
	setwritemode(XOR_PUT);
};

int msinit(int Xlo, int Xhi, int Ylo, int Yhi)
{
	int retcode;
	//_AX = 0;
	geninterrupt(0x33);
	//retcode = _AX;
	retcode = 0;
	if (retcode == 0)return 0;
	r.r_ax = 7;
	r.r_cx = Xlo; r.r_dx = Xhi;
	intr(0x33, &r);
	r.r_ax = 8;
	r.r_cx = Ylo; r.r_dx = Yhi;
	intr(0x33, &r);
	return retcode;
};


void m_kur(int X, int Y)
{
	setcolor(7);
	line(X, Y, X, Y + 21);
	line(X, Y + 21, X + 5, Y + 16);
	line(X + 5, Y + 15, X + 11, Y + 24);
	line(X + 11, Y + 24, X + 16, Y + 22);
	line(X + 16, Y + 22, X + 8, Y + 13);
	line(X + 8, Y + 13, X + 16, Y + 13);
	line(X + 16, Y + 12, X, Y);
};


int msread(int* pX, int* pY, int* pbuttons)
{
	static int X0 = -1, Y0 = -1, but0 = -1;
	do
	{
		r.r_ax = 3;
		intr(0x33, &r);
		*pX = r.r_cx;
		*pY = r.r_dx;
		*pbuttons = r.r_bx;

	} while (*pX == X0 && *pY == Y0 && *pbuttons == but0);
	if (but0);
	else m_kur(X0, Y0);
	X0 = *pX; Y0 = *pY; but0 = *pbuttons;
	return -1;
};


