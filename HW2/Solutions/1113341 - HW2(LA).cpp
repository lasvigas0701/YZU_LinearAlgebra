#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
using namespace std;
const double rad = 0.01745329252;

class Matrix {
public:
	double element[4][4]{};
	Matrix operator*(Matrix right)
	{
		Matrix product;
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				for (int k = 0; k < 4; k++)
					product.element[i][j] += this->element[i][k] * right.element[k][j];

		return product;
	}
	Matrix& operator=(Matrix right)
	{
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				this->element[i][j] = right.element[i][j];

		return *this;
	}
};

/*read every v in question(b) as a vector<double>, then push them into a vector<vector<double>>*/
void setVectorsOfB(vector<vector<double>>& vOfB, fstream& inFile1)
{
	for (int i = 0; i < 4; i++)
	{
		vector<double> v;
		for (int j = 0; j < 3; j++)
		{
			double element;
			inFile1 >> element;
			v.push_back(element);
		}
		v.push_back(1); /*in question (b), v is {Vx, Vy, Vz, 1}*/
		vOfB.push_back(v);
		v.clear();
	}
}

void setU(vector<double>& u, fstream& inFile1)
{
	for (int j = 0; j < 3; j++)
	{
		double element;
		inFile1 >> element;
		u.push_back(element);
	}
	u.push_back(1); /*in question(d), u is {Ux, Uy, Uz, 1}*/
}

void makeT(vector<Matrix> matrices, Matrix& T)
{
	while (matrices.size() != 1)
	{
		Matrix right = matrices.front();
		matrices.erase(matrices.begin());
		Matrix left = matrices.front();
		matrices.erase(matrices.begin());
		Matrix temp = left * right;
		matrices.insert(matrices.begin(), temp);
	}
	T = matrices[0];
}

void setTransformMatrix(vector<Matrix>& matrices, Matrix& T, fstream& inFile1)
{
	int n;
	inFile1 >> n;
	string str;
	for (int i = 0; i < n; i++)
	{
		Matrix matrix;
		inFile1 >> str;
		if (str[1] == 'T') /*translation*/
		{
			double ele[3] = {};
			for (int j = 0; j < 3; j++)
			{
				inFile1 >> ele[j];
			}
			Matrix t = { 1, 0, 0, ele[0],
						0, 1, 0, ele[1],
						0, 0, 1, ele[2],
						0, 0, 0, 1 };
			matrices.push_back(t);
		}
		else if (str[1] == 'P') /*projection*/
		{
			if (str[2] == 'x' && str[3] == 'y') /*Pxy => onto xy-axis*/
			{
				Matrix p = { 1, 0, 0, 0,
							0, 1, 0, 0,
							0, 0, 0, 0,
							0, 0, 0, 1 };
				matrices.push_back(p);
			}
			else if (str[2] == 'y') /*Pyz => onto yz-axis*/
			{
				Matrix p = { 0, 0, 0, 0,
							0, 1, 0, 0,
							0, 0, 1, 0,
							0, 0, 0, 1 };
				matrices.push_back(p);
			}
			else if (str[2] == 'x' && str[3] == 'z') /*Pxz => onto xz-axis*/
			{
				Matrix p = { 1, 0, 0, 0,
							0, 0, 0, 0,
							0, 0, 1, 0,
							0, 0, 0, 1 };
				matrices.push_back(p);
			}
		}
		else if (str[1] == 'S') /*scaling*/
		{
			double center[3] = {};
			double k[3] = {};
			for (int j = 0; j < 3; j++)
			{
				inFile1 >> center[j];
			}
			for (int j = 0; j < 3; j++)
			{
				inFile1 >> k[j];
			}

			/*move to origin O(0, 0, 0)*/
			Matrix o = { 1, 0, 0, -center[0],
						0, 1, 0, -center[1],
						0, 0, 1, -center[2],
						0, 0, 0, 1 };

			/*contraction & dilation, than move to the given center(center[0], center[1], center[2])*/
			Matrix b = { k[0], 0, 0, center[0],
						0, k[1], 0, center[1],
						0, 0, k[2], center[2],
						0, 0, 0, 1 };

			Matrix s = b * o;
			matrices.push_back(s);
		}
		else if (str[1] == 'R') /*rotation*/
		{
			double center[3] = {};
			double angle;
			for (int j = 0; j < 3; j++)
			{
				inFile1 >> center[j];
			}
			inFile1 >> angle;

			/*move to origin O(0, 0, 0)*/
			Matrix o = { 1, 0, 0, -center[0],
						0, 1, 0, -center[1],
						0, 0, 1, -center[2],
						0, 0, 0, 1 };

			/*rotation*/
			if (str[2] == 'x')
			{
				Matrix tempR = { 1, 0, 0, center[0],
								0, cos(angle * rad), -sin(angle * rad), center[1],
								0, sin(angle * rad), cos(angle * rad), center[2],
								0, 0, 0, 1 };
				matrix = tempR * o;
			}
			else if (str[2] == 'y')
			{
				Matrix tempR = { cos(angle * rad), 0, sin(angle * rad), center[0],
								0, 1, 0, center[1],
								-sin(angle * rad), 0, cos(angle * rad), center[2],
								0, 0, 0, 1 };
				matrix = tempR * o;
			}
			else
			{
				Matrix tempR = { cos(angle * rad), -sin(angle * rad), 0, center[0],
								sin(angle * rad), cos(angle * rad), 0, center[1],
								0, 0, 1, center[2],
								0, 0, 0, 1 };
				matrix = tempR * o;
			}

			matrices.push_back(matrix);
		}
		else if (str[1] == 'H') /*sheering*/
		{
			double center[3] = {};
			double s, t;
			for (int j = 0; j < 3; j++)
			{
				inFile1 >> center[j];
			}
			inFile1 >> s >> t;

			/*move to origin O(0, 0, 0)*/
			Matrix o = { 1, 0, 0, -center[0],
						0, 1, 0, -center[1],
						0, 0, 1, -center[2],
						0, 0, 0, 1 };

			/*sheering and move back*/
			if (str[2] == 'x')
			{
				Matrix tempS = { 1, s, t, center[0],
								0, 1, 0, center[1],
								0, 0, 1, center[2],
								0, 0, 0, 1 };
				matrices.push_back(tempS * o);
			}
			else if (str[2] == 'y')
			{
				Matrix tempS = { 1, 0, 0, center[0],
								s, 1, t, center[1],
								0, 0, 1, center[2],
								0, 0, 0, 1 };
				matrices.push_back(tempS * o);
			}
			else
			{
				Matrix tempS = { 1, 0, 0, center[0],
								0, 1, 0, center[1],
								s, t, 1, center[2],
								0, 0, 0, 1 };
				matrices.push_back(tempS * o);
			}
		}
		else if (str[1] == 'M')
		{
			Matrix m;
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					inFile1 >> m.element[i][j];
			matrices.push_back(m);
		}
	}

	makeT(matrices, T);
}

void compute_b(vector<vector<double>> b, vector<vector<double>>& ansB, Matrix T)
{
	for (int i = 0; i < b.size(); i++)
	{
		vector<double> temp;
		for (int j = 0; j < 4; j++)
		{
			double element = 0;
			for (int k = 0; k < 4; k++)
				element += T.element[j][k] * b[i][k];
			temp.push_back(element);
		}
		ansB.push_back(temp);
	}
}

/*4 points a, b, c, d, the volume is (abs{(a-d)•[(b-d)×(c-d)]} / 6) */
double countVolume(const vector<vector<double>> vec)
{
	double a_d[3]{}, b_d[3]{}, c_d[3]{}, product[3]{}, dot{};
	for (int i = 0; i < 3; i++)
	{
		a_d[i] = vec[0][i] - vec[3][i];
		b_d[i] = vec[1][i] - vec[3][i];
		c_d[i] = vec[2][i] - vec[3][i];
	}

	/*outer product*/
	for (int i = 0, l = 1, r = 2; i < 3; i++, l++, r++)
	{
		if (l > 2) l = 0;
		else if (r > 2) r = 0;
		product[i] = b_d[l] * c_d[r] - b_d[r] * c_d[l];
	}

	/*inner(dot) product*/
	for (int i = 0; i < 3; i++)
	{
		dot += a_d[i] * product[i];
	}

	return abs(dot / 6);
}

Matrix upper_triangle(Matrix T, int& count)
{
	count = 0;
	Matrix U = T;
	for (int i = 0; i < 4; i++) 
	{
		if (U.element[i][i] == 0)
		{
			for (int tempI = i + 1; tempI < 4; tempI++)
			{
				if (U.element[tempI][i] != 0) /*exchange two row*/
				{
					for (int j = 0; j < 4; j++)
					{
						double temp = U.element[i][j];
						U.element[i][j] = U.element[tempI][j];
						U.element[tempI][j] = temp;
					}
				}
			}
			count++;
		}

		for (int tempI = i + 1; tempI < 4; tempI++)
		{
			double sub = U.element[tempI][i] / U.element[i][i];
			for (int j = 0; j < 4; j++)
				U.element[tempI][j] -= sub * U.element[i][j];
		}
	}

	return U;
}

double determinant(Matrix T)
{
	int count = 0;
	Matrix U = upper_triangle(T, count);
	double det = 1;
	for (int i = 0; i < 4; i++)
		det *= U.element[i][i];
	if (isnan(det)) return 0;
	return ((count & 1) ? -det : det);
}

bool invertible(double det)
{
	return det;
}

void elimination(Matrix& T, Matrix& inverseT, int i)
{
	double base = T.element[i][i];
	for (int j = 0; j < 4; j++) /*set 1 (pivot)*/
		T.element[i][j] /= base, inverseT.element[i][j] /= base;
	for (int row = 0; row < 4; row++)
	{
		if (row == i) continue; /*set zero for not pivot items*/
		double base = T.element[row][i];
		for (int col = 0; col < 4; col++)
		{
			T.element[row][col] -= base * T.element[i][col];
			inverseT.element[row][col] -= base * inverseT.element[i][col];
		}
	}
}

void inverse(Matrix T, Matrix& inverseT)
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			inverseT.element[i][j] = (i == j ? 1 : 0);

	for (int i = 0; i < 4; i++)
	{
		
		if (T.element[i][i] == 0)
		{
			for (int j = 0; j < 4; j++)
			{
				T.element[i][j] += T.element[i + 1][j];
				inverseT.element[i][j] += inverseT.element[i][j];
			}

			elimination(T, inverseT, i);
		}
		else
		{
			elimination(T, inverseT, i);
		}
	}
}

void matrix_vector_product(Matrix T, vector<double> in, vector<double>& out)
{
	for (int i = 0; i < 4;i++)
	{
		double element = 0;
		for (int j = 0; j < 4; j++)
			element += T.element[i][j] * in[j];
		out.push_back(element);
	}
}

void output1_T(vector<Matrix> TrMatrices, Matrix& tr, fstream& outFile1)
{
	for (int i = 0; i < 4; i++)
	{
		outFile1 << fixed << setprecision(2);
		outFile1 << tr.element[i][0];
		for (int j = 1; j < 4; j++)
			outFile1 << ' ' << tr.element[i][j];
		outFile1 << endl;
	}
}

void output1_b(Matrix T, vector<vector<double>> B, fstream& outFile1)
{
	for (int i = 0; i < B.size(); i++)
	{
		vector<double> ansB;
		matrix_vector_product(T, B[i], ansB);
		outFile1 << ansB[0];
		for (int j = 1; j < 4; j++)
			outFile1 << ' ' << ansB[j];
		outFile1 << endl;
	}
}

void output1_compare(double r, double det, fstream& outFile1)
{
	outFile1 << r << ' ' << det << endl;

	if (round(r * 100) / 100 == round(det * 100) / 100 && 
		(round(r * 100) / 100 == 0 || round(det * 100) / 100 == 0))
		outFile1 << "zeros\n";
	else if (round(r * 100) / 100 == round(det * 100) / 100)
		outFile1 << "r==det(T)\n";
	else if (round(r * 100) / 100 == -(round(det * 100) / 100))
		outFile1 << "r==-det(T)\n";
	else
		outFile1 << "others\n";
}


void output1_d(Matrix T, vector<double> u, double det, fstream& outFile1)
{
	if (!invertible(det))
	{
		outFile1 << "NaN\n";
		return;
	}
	Matrix inverse_T;
	inverse(T, inverse_T);
	
	vector<double> ansU;
	matrix_vector_product(inverse_T, u, ansU);

	outFile1 << ansU[0];
	for (int i = 1; i < ansU.size(); i++)
		outFile1 << ' ' << ansU[i];
	outFile1 << endl;
}

void setPixel(vector<vector<vector<double>>>& pixel, fstream& inFile2, int l, int w, int h)
{
	for (int z = 0; z < h; z++)
	{
		vector<vector<double>> plane;
		for (int y = 0; y < l; y++)
		{
			vector<double> line;
			for (int x = 0; x < w; x++)
			{
				double element;
				inFile2 >> element;
				line.push_back(element);
			}
			plane.push_back(line);
		}
		pixel.push_back(plane);
	}
}

int getPixel(vector<vector<vector<double>>> pixel, int x, int y, int z, int l, int w, int h)
{
	if (x > l - 1 || x < 0 || y > w - 1 || y < 0 || z > h - 1 || z < 0)
		return 0;
	return pixel[z][x][y];
}

int interpolation(vector<vector<vector<double>>> pixel, int X, int x, int Y, int y, int Z, int z,
	double actX, double actY, double actZ, int l, int w, int h)
{
	double a0, a1, a2, a3, b0, b1;
	a0 = getPixel(pixel, x, y, z, l, w, h) * (X - actX) + getPixel(pixel, X, y, z, l, w, h) * (actX - x);
	a1 = getPixel(pixel, x, Y, z, l, w, h) * (X - actX) + getPixel(pixel, X, Y, z, l, w, h) * (actX - x);
	a2 = getPixel(pixel, x, y, Z, l, w, h) * (X - actX) + getPixel(pixel, X, y, Z, l, w, h) * (actX - x);
	a3 = getPixel(pixel, x, Y, Z, l, w, h) * (X - actX) + getPixel(pixel, X, Y, Z, l, w, h) * (actX - x);
	b0 = a0 * (Y - actY) + a1 * (actY - y);
	b1 = a2 * (Y - actY) + a3 * (actY - y);

	return floor(b0 * (Z - actZ) + b1 * (actZ - z));
}

void compute_part2(vector<double> pos, Matrix T, vector<vector<vector<double>>> pixel, 
	int*** output2, int h, int l, int w)
{
	vector<double> ori;
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < l; j++)
		{
			for (int k = 0; k < w; k++)
			{
				pos.push_back(j); /*x*/
				pos.push_back(k); /*y*/
				pos.push_back(i); /*z*/
				pos.push_back(1);
				matrix_vector_product(T, pos, ori);
				int x = floor(ori[0]), X = ceil(ori[0]),
					y = floor(ori[1]), Y = ceil(ori[1]),
					z = floor(ori[2]), Z = ceil(ori[2]);
				output2[i][j][k] = interpolation(pixel, X, x, Y, y, Z, z, ori[0], ori[1], ori[2], l, w, h);
				pos.clear(), ori.clear();
			}
		}
	}
}

void output_2(int*** output2, fstream& outFile2, int h, int l, int w)
{
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < l; j++)
		{
			outFile2 << output2[i][j][0];
			for (int k = 1; k < w; k++)
				outFile2 << ' ' << output2[i][j][k];
			outFile2 << endl;
		}
	}
}

int main(int argc, char** argv) {
	/*===== Part 1 =====*/
	fstream inFile1(argv[1], ios::in);

	vector<vector<double>> vOfQuestionB, ansB;
	setVectorsOfB(vOfQuestionB, inFile1);

	vector<double> u;
	setU(u, inFile1);

	vector<Matrix> matrices;
	Matrix T;
	setTransformMatrix(matrices, T, inFile1);
	compute_b(vOfQuestionB, ansB, T);

	double volumeV = countVolume(vOfQuestionB), volumeU = countVolume(ansB);
	double r = volumeU / volumeV, det = round(determinant(T) * 100) / 100;  /*if det is 0.000001, should print "NaN"*/
	
	fstream outFile1(argv[2], ios::out);
	output1_T(matrices, T, outFile1);
	output1_b(T, vOfQuestionB, outFile1);
	output1_compare(r, det, outFile1);
	output1_d(T, u, det, outFile1);

	/*===== Part 2 =====*/
	fstream inFile2(argv[3], ios::in);
	int l, w, h;
	inFile2 >> l >> w >> h;
	vector<vector<vector<double>>> pixel;
	setPixel(pixel, inFile2, l, w, h);
	vector<Matrix> matrices2;
	Matrix T2, inverseT2;
	setTransformMatrix(matrices2, T2, inFile2);
	inverse(T2, inverseT2);
	
	/*bulid 3-dimensional array to output*/
	int*** output2 = new int** [h];
	for (int i = 0; i < h; i++)
	{
		output2[i] = new int* [l];
		for (int j = 0; j < l; j++)
			output2[i][j] = new int[w] {};
	}

	vector<double> pos;
	compute_part2(pos, inverseT2, pixel, output2, h, l, w);

	fstream outFile2(argv[4], ios::out);
	output_2(output2, outFile2, h, l, w);

	/*release the output array*/
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < l; j++)
			delete[] output2[i][j];
		delete[] output2[i];
	}
	delete[] output2;
}