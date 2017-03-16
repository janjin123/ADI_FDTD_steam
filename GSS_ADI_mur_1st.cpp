#include"GSS_ADI_mur_1st.h"

//============================总调度============================//
void adi_fdtd_leapforg_mur1_GSS(Grid* halfgrid_now)

{
	//=========================输出文件声明=========================//
	ofstream file_mur1("result\\temp_mur1_1500_5_5_5.txt");
	ofstream file_mur1_plat("result\\platform_mur1_1500_5_5_5.txt");

	int step = 0;//时间步长

	int result_z = 5;
	int result_x = 5;
	int result_y = 5;

	init_source(halfgrid_now);//初始化输入源	

	while (step < STEPS)
	{
		//基本思想：分别计算六个参量全网格的值，由于计算不需要当前时刻的值，可以分步全局计算

		gss_cal_mur1(halfgrid_now, step);

		//==========================保存结果==========================//
		file_mur1 << step << '\t' << halfgrid_now[result_z * Nx*Ny + result_x * Ny + result_y].ex
			<< '\t' << halfgrid_now[result_z * Nx*Ny + result_x * Ny + result_y].ey
			<< '\t' << halfgrid_now[result_z * Nx*Ny + result_x * Ny + result_y].ez << '\t';

		file_mur1 << halfgrid_now[result_z * Nx*Ny + result_x * Ny + result_y].bx
			<< '\t' << halfgrid_now[result_z * Nx*Ny + result_x * Ny + result_y].by
			<< '\t' << halfgrid_now[result_z * Nx*Ny + result_x * Ny + result_y].bz << '\t';

		file_mur1 << '\n';

		cout << "Step--- " << step << " ---has finished." << endl;
		step++;

	}//while
	file_mur1.close();

	//=======================输出横截面数据========================//
	for (int i = 0; i < Nx - 1; i++)
	{
		for (int k = 0; k < Nz - 1; k++)
		{
			file_mur1_plat << k << '\t' << i << '\t'
				<< halfgrid_now[k*Nx*Ny + i*Ny + 5].ey << '\t'
				<< halfgrid_now[k*Nx*Ny + i*Ny + 5].bx << '\t'
				<< halfgrid_now[k*Nx*Ny + i*Ny + 5].bz << endl;
		}
	}

	file_mur1_plat.close();

}//函数结尾

 //=========================gss计算调用=========================//
void gss_cal_mur1(Grid* halfgrid_now, int step)
{
	//-------------------------------------------------------------------计算电场

	//基本思想：分别计算六个参量全网格的值，由于计算不需要当前时刻的值，可以分步全局计算

	mur1_gsscalc_ez(halfgrid_now, step);
	mur1_gsscalc_ex(halfgrid_now, step);
	mur1_gsscalc_ey(halfgrid_now, step);

	//-------------------------------------------------------------------计算磁场
	mur1_gsscalc_bz(halfgrid_now, step);
	mur1_gsscalc_bx(halfgrid_now, step);
	mur1_gsscalc_by(halfgrid_now, step);

}

//========================gss求解的实现========================//
void mur1_gsscalc_ez(Grid* halfgrid_now, int step)
{
	double aez = (-1 * (dt / 2)*(dt / 2)*(1 / mur0)*(1 / dx)*(1 / dx));
	double bez = (epsl0 + 2 * (dt / 2)*(dt / 2)*(1 / mur0)*(1 / dx)*(1 / dx));
	double cez = aez;

	int nRet;//GSS函数的返回值
	int N = Nx - 1;

	int nnz = 3 * N - 2;
	int nRow = N;
	int nCol = N;

	int ptr[Nx];
	int ind[3 * (Nx - 1) - 2];
	double val[3 * (Nx - 1) - 2];
	double rhs[Nx - 1];

	void *hSolver = NULL;//求解器指针
	double setting[32];
	for (int i = 0; i < 32; i++)	setting[i] = 0.0;//配置参数初始化
	int type = 0;

	//处理ptr数组
	ptr[0] = 0;
	ptr[1] = 2;
	ptr[N] = 3 * N - 2;
	for (int i = 2; i < N; i++)
	{
		ptr[i] = ptr[i - 1] + 3;
	}

	//处理ind数组
	ind[0] = 0;
	ind[1] = 1;
	ind[3 * N - 3] = N - 1;
	ind[3 * N - 4] = N - 2;
	for (int i = 2, j = 0; i + 2 < 3 * N - 4; i = i + 3)
	{
		ind[i] = j;
		ind[i + 1] = j + 1;
		ind[i + 2] = j + 2;
		j++;
	}
	//val数组处理
	val[0] = bez;
	val[2] = cez;
	val[3 * N - 3] = bez;
	val[3 * N - 5] = aez;
	for (int i = 1, j = 2; i + 3 < 3 * N - 2; i = i + 3)
	{
		val[i] = aez;
		val[i + 2] = bez;
		val[i + 4] = cez;
		j++;
	}
	//rhs数组初始化
	for (int i = 0; i < N; i++)
		rhs[i] = 0.0;
	//生成系数矩阵

	for (int k = 0; k < Nz - 1; k++)
	{
		for (int j = 0; j < Ny - 1; j++)
		{
			for (int i = 0; i < Nx - 1; i++)
			{
				//首先处理矩阵方程的右端项rhs数组
				if (i == 0 && j != 0)
					rhs[i] = bez*halfgrid_now[k*Nx*Ny + i*Ny + j].ez + cez*halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].ez
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].by) / dx - (halfgrid_now[k*Nx*Ny + i*Ny + j].bx - halfgrid_now[k*Nx*Ny + i*Ny + j - 1].bx) / dy);
				else if (j == 0 && i != 0)
					rhs[i] = aez*halfgrid_now[k*Nx*Ny + (i - 1)*Ny + j].ez + bez*halfgrid_now[k*Nx*Ny + i*Ny + j].ez + cez*halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].ez
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].by - halfgrid_now[k*Nx*Ny + (i - 1)*Ny + j].by) / dx - (halfgrid_now[k*Nx*Ny + i*Ny + j].bx) / dy);
				else if (i == 0 && j == 0)
					rhs[i] = bez*halfgrid_now[k*Nx*Ny + i*Ny + j].ez + cez*halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].ez
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].by) / dx - (halfgrid_now[k*Nx*Ny + i*Ny + j].bx) / dy);
				else if (i != 0 && j != 0)
					rhs[i] = aez*halfgrid_now[k*Nx*Ny + (i - 1)*Ny + j].ez + bez*halfgrid_now[k*Nx*Ny + i*Ny + j].ez + cez*halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].ez
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].by - halfgrid_now[k*Nx*Ny + (i - 1)*Ny + j].by) / dx - (halfgrid_now[k*Nx*Ny + i*Ny + j].bx - halfgrid_now[k*Nx*Ny + i*Ny + j - 1].bx) / dy);
			}


			//GSS求解
			nRet = GSS_init_ld(nRow, nCol, ptr, ind, val, type, setting);
			if (nRet != GRUS_OK) {
				printf("\tERROR at init GSS solver. ERROR CODE:%d\r\n", nRet);
				return;
			}

			hSolver = GSS_symbol_ld(nRow, nCol, ptr, ind, val);
			if (hSolver == NULL) {
				printf("\tERROR at SYMBOLIC ANALYSIS.\r\n");
				exit(0);
			}

			nRet = GSS_numeric_ld(nRow, nCol, ptr, ind, val, hSolver);
			if (nRet != GRUS_OK) {
				printf("\r\n\tERROR at NUMERIC FACTORIZATION. ERROR CODE:%d\r\n", nRet);
				hSolver = NULL;		//必须设置为NULL,GSS已自动释放内存
				exit(0);
			}

			GSS_solve_ld(hSolver, nRow, nCol, ptr, ind, val, rhs);

			for (int i1 = 0; i1 < Nx - 1; i1++)
			{
				//对结果进行修正								
				if (j == 0 || j == Ny - 2 || i1 == 0 || i1 == Nx - 2)//处理ez的边界问题，在四个面的位置应该为0
				{
					rhs[i1] = 0.0;
				}
				//保存结果	
				halfgrid_now[k*Nx*Ny + i1*Ny + j].ez = rhs[i1];
			}
			if (hSolver != NULL)
				GSS_clear_ld(hSolver);

		}
	}
}

void mur1_gsscalc_ex(Grid* halfgrid_now, int step)
{
	double aex = (-1 * (dt / 2)*(dt / 2)*(1 / mur0)*(1 / dy)*(1 / dy));
	double bex = (epsl0 + 2 * (dt / 2)*(dt / 2)*(1 / mur0)*(1 / dy)*(1 / dy));
	double cex = aex;

	int nRet;//GSS函数的返回值
	int N = Ny - 1;
	int nnz = 3 * N - 2;
	int nRow = N;
	int nCol = N;

	int ptr[Ny];
	int ind[3 * (Ny - 1) - 2];
	double val[3 * (Ny - 1) - 2];
	double rhs[Ny - 1];

	void *hSolver = NULL;//求解器指针
	double setting[32];
	for (int i = 0; i < 32; i++)	setting[i] = 0.0;//配置参数初始化
	int type = 0;

	//处理ptr数组
	ptr[0] = 0;
	ptr[1] = 2;
	ptr[N] = 3 * N - 2;
	for (int i = 2; i < N; i++)
	{
		ptr[i] = ptr[i - 1] + 3;
	}
	//处理ind数组
	ind[0] = 0;
	ind[1] = 1;
	ind[3 * N - 3] = N - 1;
	ind[3 * N - 4] = N - 2;
	for (int i = 2, j = 0; i + 2 < 3 * N - 4; i = i + 3)
	{
		ind[i] = j;
		ind[i + 1] = j + 1;
		ind[i + 2] = j + 2;
		j++;
	}
	//val数组处理
	val[0] = bex;
	val[2] = cex;
	val[3 * N - 3] = bex;
	val[3 * N - 5] = aex;
	for (int i = 1, j = 2; i + 3 < 3 * N - 2; i = i + 3)
	{
		val[i] = aex;
		val[i + 2] = bex;
		val[i + 4] = cex;
		j++;
	}
	//rhs数组初始化
	for (int i = 0; i < N; i++)
		rhs[i] = 0.0;

	for (int k = 0; k < Nz - 1; k++)
	{
		for (int i = 0; i < Nx - 1; i++)
		{
			for (int j = 0; j < Ny - 1; j++)
			{
				//首先处理矩阵方程的右端项rhs数组
				if (j == 0 && k != 0)
					rhs[j] = bex*halfgrid_now[k*Nx*Ny + i*Ny + j].ex + cex*halfgrid_now[k*Nx*Ny + i*Ny + j + 1].ex
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].bz) / dy - (halfgrid_now[k*Nx*Ny + i*Ny + j].by - halfgrid_now[(k - 1)*Nx*Ny + i*Ny + j].by) / dz);
				else if (k == 0 && j != 0)
					rhs[j] = aex*halfgrid_now[k*Nx*Ny + i*Ny + j - 1].ex + bex*halfgrid_now[k*Nx*Ny + i*Ny + j].ex + cex*halfgrid_now[k*Nx*Ny + i*Ny + j + 1].ex
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].bz - halfgrid_now[k*Nx*Ny + i*Ny + j - 1].bz) / dy - (halfgrid_now[k*Nx*Ny + i*Ny + j].by) / dz);
				else if (j == 0 && k == 0)
					rhs[j] = bex*halfgrid_now[k*Nx*Ny + i*Ny + j].ex + cex*halfgrid_now[k*Nx*Ny + i*Ny + j + 1].ex
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].bz) / dy - (halfgrid_now[k*Nx*Ny + i*Ny + j].by) / dz);
				else if (j != 0 && k != 0)
					rhs[j] = aex*halfgrid_now[k*Nx*Ny + i*Ny + j - 1].ex + bex*halfgrid_now[k*Nx*Ny + i*Ny + j].ex + cex*halfgrid_now[k*Nx*Ny + i*Ny + j + 1].ex
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].bz - halfgrid_now[k*Nx*Ny + i*Ny + j - 1].bz) / dy - (halfgrid_now[k*Nx*Ny + i*Ny + j].by - halfgrid_now[(k - 1)*Nx*Ny + i*Ny + j].by) / dz);
			}//GSS求解
			nRet = GSS_init_ld(nRow, nCol, ptr, ind, val, type, setting);
			if (nRet != GRUS_OK) {
				printf("\tERROR at init GSS solver. ERROR CODE:%d\r\n", nRet);
				return;
			}

			hSolver = GSS_symbol_ld(nRow, nCol, ptr, ind, val);
			if (hSolver == NULL) {
				printf("\tERROR at SYMBOLIC ANALYSIS.\r\n");
				exit(0);
			}

			nRet = GSS_numeric_ld(nRow, nCol, ptr, ind, val, hSolver);
			if (nRet != GRUS_OK) {
				printf("\r\n\tERROR at NUMERIC FACTORIZATION. ERROR CODE:%d\r\n", nRet);
				hSolver = NULL;		//必须设置为NULL,GSS已自动释放内存
				exit(0);
			}

			GSS_solve_ld(hSolver, nRow, nCol, ptr, ind, val, rhs);

			for (int j1 = 0; j1 < Ny - 1; j1++)
			{
				//对结果进行修正				
				if (k == Nz - 2)
				{
					rhs[j1] = halfgrid_now[(k - 1)*Nx*Ny + i*Ny + j1].ex //p0--->k*Nx*Ny + i*Ny + j1, Q0----->(k-1)*Nx*Ny + i*Ny + j1
						+ ((c*dt - dz) / (c*dt + dz))*(halfgrid_now[(k - 1)*Nx*Ny + i*Ny + j1].ex - halfgrid_now[k*Nx*Ny + i*Ny + j1].ex);
				}

				if (j1 == 0 || j1 == Ny - 2 || k == 0)
				{
					rhs[j1] = 0.0;
				}

				halfgrid_now[k*Nx*Ny + i*Ny + j1].ex = rhs[j1];
			}

			if (hSolver != NULL)
				GSS_clear_ld(hSolver);
		}

	}
}

void mur1_gsscalc_ey(Grid* halfgrid_now, int step)
{
	double aey = (-1 * (dt / 2)*(dt / 2)*(1 / mur0)*(1 / dz)*(1 / dz));
	double bey = (epsl0 + 2 * (dt / 2)*(dt / 2)*(1 / mur0)*(1 / dz)*(1 / dz));
	double cey = aey;

	int nRet;//GSS函数的返回值
	int N = Nz - 1;
	int nnz = 3 * N - 2;
	int nRow = N;
	int nCol = N;

	int ptr[Nz];
	int ind[3 * (Nz - 1) - 2];
	double val[3 * (Nz - 1) - 2];
	double rhs[Nz - 1];

	void *hSolver = NULL;//求解器指针
	double setting[32];
	for (int i = 0; i < 32; i++)	setting[i] = 0.0;//配置参数初始化
	int type = 0;

	//处理ptr数组
	ptr[0] = 0;
	ptr[1] = 2;
	ptr[N] = 3 * N - 2;
	for (int i = 2; i < N; i++)
	{
		ptr[i] = ptr[i - 1] + 3;
	}
	//处理ind数组
	ind[0] = 0;
	ind[1] = 1;
	ind[3 * N - 3] = N - 1;
	ind[3 * N - 4] = N - 2;
	for (int i = 2, j = 0; i + 2 < 3 * N - 4; i = i + 3)
	{
		ind[i] = j;
		ind[i + 1] = j + 1;
		ind[i + 2] = j + 2;
		j++;
	}
	//val数组处理
	val[0] = bey;
	val[2] = cey;
	val[3 * N - 3] = bey;
	val[3 * N - 5] = aey;
	for (int i = 1, j = 2; i + 3 < 3 * N - 2; i = i + 3)
	{
		val[i] = aey;
		val[i + 2] = bey;
		val[i + 4] = cey;
		j++;
	}
	//rhs数组初始化
	for (int i = 0; i < N; i++)
		rhs[i] = 0.0;

	for (int i = 0; i < Nx - 1; i++)
	{
		for (int j = 0; j < Ny - 1; j++)
		{
			for (int k = 0; k < Nz - 1; k++)
			{
				if (k == 0 && i != 0)
					rhs[k] = bey*halfgrid_now[k*Nx*Ny + i*Ny + j].ey + cey*halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].ey
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].bx) / dz - (halfgrid_now[k*Nx*Ny + i*Ny + j].bz - halfgrid_now[k*Nx*Ny + (i - 1)*Ny + j].bz) / dx);
				else if (i == 0 && k != 0)
					rhs[k] = aey*halfgrid_now[(k - 1)*Nx*Ny + i*Ny + j].ey + bey*halfgrid_now[k*Nx*Ny + i*Ny + j].ey + cey*halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].ey
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].bx - halfgrid_now[(k - 1)*Nx*Ny + i*Ny + j].bx) / dz - (halfgrid_now[k*Nx*Ny + i*Ny + j].bz) / dx);
				else if (k == 0 && i == 0)
					rhs[k] = bey*halfgrid_now[k*Nx*Ny + i*Ny + j].ey + cey*halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].ey
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].bx) / dz - (halfgrid_now[k*Nx*Ny + i*Ny + j].bz) / dx);
				else if (k != 0 && i != 0)
					rhs[k] = aey*halfgrid_now[(k - 1)*Nx*Ny + i*Ny + j].ey + bey*halfgrid_now[k*Nx*Ny + i*Ny + j].ey + cey*halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].ey
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j].bx - halfgrid_now[(k - 1)*Nx*Ny + i*Ny + j].bx) / dz - (halfgrid_now[k*Nx*Ny + i*Ny + j].bz - halfgrid_now[k*Nx*Ny + (i - 1)*Ny + j].bz) / dx);
			}
			//GSS求解
			nRet = GSS_init_ld(nRow, nCol, ptr, ind, val, type, setting);
			if (nRet != GRUS_OK) {
				printf("\tERROR at init GSS solver. ERROR CODE:%d\r\n", nRet);
				return;
			}

			hSolver = GSS_symbol_ld(nRow, nCol, ptr, ind, val);
			if (hSolver == NULL) {
				printf("\tERROR at SYMBOLIC ANALYSIS.\r\n");
				exit(0);
			}

			nRet = GSS_numeric_ld(nRow, nCol, ptr, ind, val, hSolver);
			if (nRet != GRUS_OK) {
				printf("\r\n\tERROR at NUMERIC FACTORIZATION. ERROR CODE:%d\r\n", nRet);
				hSolver = NULL;//必须设置为NULL,GSS已自动释放内存
				exit(0);
			}

			GSS_solve_ld(hSolver, nRow, nCol, ptr, ind, val, rhs);
			//结果的修正与保存
			for (int k1 = 0; k1 < Nz - 1; k1++)
			{
				if (k1 == 0) //对结果进行修正
				{//不改变源的值					
					if (step*dt < 2 * T)
					{
						double rising_edge = (step*dt) / (2 * T);
						double temp_ey = ((omega*mur0*X) / pi) * hm * sin((pi / X)*i*dx);

						rhs[k1] = rising_edge * temp_ey * sin(omega*step*dt);//ey
					}
					else
					{
						double temp_ey = ((omega*mur0*X) / pi) * hm * sin((pi / X)*i*dx);

						rhs[k1] = temp_ey*sin(omega*step*dt);//ey
					}
				}
				if (k1 == Nz - 2)
				{
					rhs[k1] = halfgrid_now[(k1 - 1)*Nx*Ny + i*Ny + j].ey
						+ ((c*dt - dz) / (c*dt + dz))*(halfgrid_now[(k1 - 1)*Nx*Ny + i*Ny + j].ey - halfgrid_now[k1*Nx*Ny + i*Ny + j].ey);

				}

				if (i == 0 || i == Nx - 2)
				{
					rhs[k1] = 0.0;
				}
				//保存结果
				halfgrid_now[k1*Nx*Ny + i*Ny + j].ey = rhs[k1];
			}

			if (hSolver != NULL)
				GSS_clear_ld(hSolver);
		}
	}
}

void mur1_gsscalc_bz(Grid* halfgrid_now, int step)
{
	double ahz = (-1 * (dt / 2)*(dt / 2)*(1 / epsl0)*(1 / dx)*(1 / dx));
	double bhz = (mur0 + 2 * (dt / 2)*(dt / 2)*(1 / epsl0)*(1 / dx)*(1 / dx));
	double chz = ahz;

	int nRet;//GSS函数的返回值
	int N = Nx - 1;
	int nnz = 3 * N - 2;
	int nRow = N;
	int nCol = N;

	int ptr[Nx];
	int ind[3 * (Nx - 1) - 2];
	double val[3 * (Nx - 1) - 2];
	double rhs[Nx - 1];

	void *hSolver = NULL;//求解器指针
	double setting[32];
	for (int i = 0; i < 32; i++)	setting[i] = 0.0;//配置参数初始化
	int type = 0;

	//处理ptr数组
	ptr[0] = 0;
	ptr[1] = 2;
	ptr[N] = 3 * N - 2;
	for (int i = 2; i < N; i++)
	{
		ptr[i] = ptr[i - 1] + 3;
	}
	//处理ind数组
	ind[0] = 0;
	ind[1] = 1;
	ind[3 * N - 3] = N - 1;
	ind[3 * N - 4] = N - 2;
	for (int i = 2, j = 0; i + 2 < 3 * N - 4; i = i + 3)
	{
		ind[i] = j;
		ind[i + 1] = j + 1;
		ind[i + 2] = j + 2;
		j++;
	}
	//val数组处理
	val[0] = bhz;
	val[2] = chz;
	val[3 * N - 3] = bhz;
	val[3 * N - 5] = ahz;
	for (int i = 1, j = 2; i + 3 < 3 * N - 2; i = i + 3)
	{
		val[i] = ahz;
		val[i + 2] = bhz;
		val[i + 4] = chz;
		j++;
	}
	//rhs数组初始化
	for (int i = 0; i < N; i++)
		rhs[i] = 0.0;

	for (int k = 0; k < Nz - 1; k++)
	{
		for (int j = 0; j < Ny - 1; j++)
		{
			for (int i = 0; i < Nx - 1; i++)
			{
				//首先处理矩阵方程的右端项rhs数组
				if (i == 0)
					rhs[i] = bhz*halfgrid_now[k*Nx*Ny + i*Ny + j].bz + chz*halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].bz
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j + 1].ex - halfgrid_now[k*Nx*Ny + i*Ny + j].ex) / dy - (halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].ey - halfgrid_now[k*Nx*Ny + i*Ny + j].ey) / dx);
				else
					rhs[i] = ahz*halfgrid_now[k*Nx*Ny + (i - 1)*Ny + j].bz + bhz*halfgrid_now[k*Nx*Ny + i*Ny + j].bz + chz*halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].bz
					+ dt*((halfgrid_now[k*Nx*Ny + i*Ny + j + 1].ex - halfgrid_now[k*Nx*Ny + i*Ny + j].ex) / dy - (halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].ey - halfgrid_now[k*Nx*Ny + i*Ny + j].ey) / dx);
			}
			//GSS求解
			nRet = GSS_init_ld(nRow, nCol, ptr, ind, val, type, setting);
			if (nRet != GRUS_OK) {
				printf("\tERROR at init GSS solver. ERROR CODE:%d\r\n", nRet);
				return;
			}

			hSolver = GSS_symbol_ld(nRow, nCol, ptr, ind, val);
			if (hSolver == NULL) {
				printf("\tERROR at SYMBOLIC ANALYSIS.\r\n");
				exit(0);
			}

			nRet = GSS_numeric_ld(nRow, nCol, ptr, ind, val, hSolver);
			if (nRet != GRUS_OK) {
				printf("\r\n\tERROR at NUMERIC FACTORIZATION. ERROR CODE:%d\r\n", nRet);
				hSolver = NULL;	//必须设置为NULL,GSS已自动释放内存
				exit(0);
			}

			GSS_solve_ld(hSolver, nRow, nCol, ptr, ind, val, rhs);
			for (int i1 = 0; i1 < Nx - 1; i1++)
			{
				//对结果进行修正				
				if (k == 0)//不改变输入源的值
				{
					double rising_edge = (step*dt) / (2 * T);

					if (step*dt < 2 * T)
					{
						double temp_bz = hm * cos((pi / X) * i1*dx);

						rhs[i1] = rising_edge * temp_bz * cos(omega*step*dt);//hz
					}
					else
					{
						double temp_bz = hm * cos((pi / X) * i1 * dx);

						rhs[i1] = temp_bz * cos(omega * step * dt);//hz
					}

				}
				if (k == Nz - 2)//在右截面加源而且使其赋值为0，可以相当于理想吸收边界
				{
					/*rhs[i1] = 0.0;*/
					rhs[i1] = halfgrid_now[(k - 1)*Nx*Ny + i1*Ny + j].bz
						+ ((c*dt - dz) / (c*dt + dz))*(halfgrid_now[(k - 1)*Nx*Ny + i1*Ny + j].bz - halfgrid_now[k*Nx*Ny + i1*Ny + j].bz);

				}
				//保存结果
				halfgrid_now[k*Nx*Ny + i1*Ny + j].bz = rhs[i1];
			}

			if (hSolver != NULL)
				GSS_clear_ld(hSolver);
		}
	}

}

void mur1_gsscalc_bx(Grid* halfgrid_now, int step)
{
	double ahx = (-1 * (dt / 2)*(dt / 2)*(1 / epsl0)*(1 / dy)*(1 / dy));
	double bhx = (mur0 + 2 * (dt / 2)*(dt / 2)*(1 / epsl0)*(1 / dy)*(1 / dy));
	double chx = ahx;

	int nRet;//GSS函数的返回值
	int N = Ny - 1;
	int nnz = 3 * N - 2;
	int nRow = N;
	int nCol = N;

	int ptr[Ny];
	int ind[3 * (Ny - 1) - 2];
	double val[3 * (Ny - 1) - 2];
	double rhs[Ny - 1];

	void *hSolver = NULL;//求解器指针
	double setting[32];
	for (int i = 0; i < 32; i++)	setting[i] = 0.0;//配置参数初始化
	int type = 0;

	//处理ptr数组
	ptr[0] = 0;
	ptr[1] = 2;
	ptr[N] = 3 * N - 2;
	for (int i = 2; i < N; i++)
	{
		ptr[i] = ptr[i - 1] + 3;
	}
	//处理ind数组
	ind[0] = 0;
	ind[1] = 1;
	ind[3 * N - 3] = N - 1;
	ind[3 * N - 4] = N - 2;
	for (int i = 2, j = 0; i + 2 < 3 * N - 4; i = i + 3)
	{
		ind[i] = j;
		ind[i + 1] = j + 1;
		ind[i + 2] = j + 2;
		j++;
	}
	//val数组处理
	val[0] = bhx;
	val[2] = chx;
	val[3 * N - 3] = bhx;
	val[3 * N - 5] = ahx;
	for (int i = 1, j = 2; i + 3 < 3 * N - 2; i = i + 3)
	{
		val[i] = ahx;
		val[i + 2] = bhx;
		val[i + 4] = chx;
		j++;
	}
	//rhs数组初始化
	for (int i = 0; i < N; i++)
		rhs[i] = 0.0;

	for (int k = 0; k < Nz - 1; k++)
	{
		for (int i = 0; i < Nx - 1; i++)
		{
			for (int j = 0; j < Ny - 1; j++)
			{
				//首先处理矩阵方程的右端项rhs数组
				if (j == 0)
					rhs[j] = bhx*halfgrid_now[k*Nx*Ny + i*Ny + j].bx + chx*halfgrid_now[k*Nx*Ny + i*Ny + j + 1].bx
					+ dt*((halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].ey - halfgrid_now[k*Nx*Ny + i*Ny + j].ey) / dz - (halfgrid_now[k*Nx*Ny + i*Ny + j + 1].ez - halfgrid_now[k*Nx*Ny + i*Ny + j].ez) / dy);
				else
					rhs[j] = ahx*halfgrid_now[k*Nx*Ny + i*Ny + j - 1].bx + bhx*halfgrid_now[k*Nx*Ny + i*Ny + j].bx + chx*halfgrid_now[k*Nx*Ny + i*Ny + j + 1].bx
					+ dt*((halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].ey - halfgrid_now[k*Nx*Ny + i*Ny + j].ey) / dz - (halfgrid_now[k*Nx*Ny + i*Ny + j + 1].ez - halfgrid_now[k*Nx*Ny + i*Ny + j].ez) / dy);
			}
			//GSS求解
			nRet = GSS_init_ld(nRow, nCol, ptr, ind, val, type, setting);
			if (nRet != GRUS_OK) {
				printf("\tERROR at init GSS solver. ERROR CODE:%d\r\n", nRet);
				return;
			}

			hSolver = GSS_symbol_ld(nRow, nCol, ptr, ind, val);
			if (hSolver == NULL) {
				printf("\tERROR at SYMBOLIC ANALYSIS.\r\n");
				exit(0);
			}

			nRet = GSS_numeric_ld(nRow, nCol, ptr, ind, val, hSolver);
			if (nRet != GRUS_OK) {
				printf("\r\n\tERROR at NUMERIC FACTORIZATION. ERROR CODE:%d\r\n", nRet);
				hSolver = NULL;		//必须设置为NULL,GSS已自动释放内存
				exit(0);
			}

			GSS_solve_ld(hSolver, nRow, nCol, ptr, ind, val, rhs);

			for (int j1 = 0; j1 < Ny - 1; j1++)
			{
				//对结果进行修正
				/*if (k == 0)
				{
				rhs[j1] = -1 * (X*bate / pi)*hm*sin((pi / X)*i*dx)*sin(omega*step*dt*1.5);
				}*/
				if (i == 0 || i == Nx - 2)
				{
					rhs[j1] = 0.0;
				}
				//保存结果
				halfgrid_now[k*Nx*Ny + i*Ny + j1].bx = rhs[j1];
			}
			if (hSolver != NULL)
				GSS_clear_ld(hSolver);
		}
	}

}

void mur1_gsscalc_by(Grid* halfgrid_now, int step)
{
	double ahy = (-1 * (dt / 2)*(dt / 2)*(1 / epsl0)*(1 / dz)*(1 / dz));
	double bhy = (mur0 + 2 * (dt / 2)*(dt / 2)*(1 / epsl0)*(1 / dz)*(1 / dz));
	double chy = ahy;

	int nRet;//GSS函数的返回值
	int N = Nz - 1;
	int nnz = 3 * N - 2;
	int nRow = N;
	int nCol = N;

	int ptr[Nz];
	int ind[3 * (Nz - 1) - 2];
	double val[3 * (Nz - 1) - 2];
	double rhs[Nz - 1];

	void *hSolver = NULL;//求解器指针
	double setting[32];
	for (int i = 0; i < 32; i++)	setting[i] = 0.0;//配置参数初始化
	int type = 0;

	//处理ptr数组
	ptr[0] = 0;
	ptr[1] = 2;
	ptr[N] = 3 * N - 2;
	for (int i = 2; i < N; i++)
	{
		ptr[i] = ptr[i - 1] + 3;
	}
	//处理ind数组
	ind[0] = 0;
	ind[1] = 1;
	ind[3 * N - 3] = N - 1;
	ind[3 * N - 4] = N - 2;
	for (int i = 2, j = 0; i + 2 < 3 * N - 4; i = i + 3)
	{
		ind[i] = j;
		ind[i + 1] = j + 1;
		ind[i + 2] = j + 2;
		j++;
	}
	//val数组处理
	val[0] = bhy;
	val[2] = chy;
	val[3 * N - 3] = bhy;
	val[3 * N - 5] = ahy;
	for (int i = 1, j = 2; i + 3 < 3 * N - 2; i = i + 3)
	{
		val[i] = ahy;
		val[i + 2] = bhy;
		val[i + 4] = chy;
		j++;
	}
	//rhs数组初始化
	for (int i = 0; i < N; i++)
		rhs[i] = 0.0;

	for (int i = 0; i < Nx - 1; i++)
	{
		for (int j = 0; j < Ny - 1; j++)
		{
			for (int k = 0; k < Nz - 1; k++)
			{
				//首先处理矩阵方程的右端项rhs数组
				if (k == 0)
					rhs[k] = bhy*halfgrid_now[k*Nx*Ny + i*Ny + j].by + chy*halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].by
					+ dt*((halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].ez - halfgrid_now[k*Nx*Ny + i*Ny + j].ez) / dx - (halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].ex - halfgrid_now[k*Nx*Ny + i*Ny + j].ex) / dz);
				else
					rhs[k] = ahy*halfgrid_now[(k - 1)*Nx*Ny + i*Ny + j].by + bhy*halfgrid_now[k*Nx*Ny + i*Ny + j].by + chy*halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].by
					+ dt*((halfgrid_now[k*Nx*Ny + (i + 1)*Ny + j].ez - halfgrid_now[k*Nx*Ny + i*Ny + j].ez) / dx - (halfgrid_now[(k + 1)*Nx*Ny + i*Ny + j].ex - halfgrid_now[k*Nx*Ny + i*Ny + j].ex) / dz);
			}
			//GSS求解
			nRet = GSS_init_ld(nRow, nCol, ptr, ind, val, type, setting);
			if (nRet != GRUS_OK) {
				printf("\tERROR at init GSS solver. ERROR CODE:%d\r\n", nRet);
				return;
			}

			hSolver = GSS_symbol_ld(nRow, nCol, ptr, ind, val);
			if (hSolver == NULL) {
				printf("\tERROR at SYMBOLIC ANALYSIS.\r\n");
				exit(0);
			}

			nRet = GSS_numeric_ld(nRow, nCol, ptr, ind, val, hSolver);
			if (nRet != GRUS_OK) {
				printf("\r\n\tERROR at NUMERIC FACTORIZATION. ERROR CODE:%d\r\n", nRet);
				hSolver = NULL;		//必须设置为NULL,GSS已自动释放内存
				exit(0);
			}

			GSS_solve_ld(hSolver, nRow, nCol, ptr, ind, val, rhs);

			for (int k1 = 0; k1 < Nz - 1; k1++)
			{
				//对结果进行修正
				/*if (k1 == 0)
				{
				rhs[k1] = 0.0;
				}*/
				if (j == 0 || j == Ny - 2)
				{
					rhs[k1] = 0.0;
				}

				//保存结果
				halfgrid_now[k1*Nx*Ny + i*Ny + j].by = rhs[k1];

			}
			if (hSolver != NULL)
				GSS_clear_ld(hSolver);
		}
	}
}

