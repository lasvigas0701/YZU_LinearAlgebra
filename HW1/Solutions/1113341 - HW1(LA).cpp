#include <iostream>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <vector>
#include <iomanip>
using namespace std;
using namespace cv;

int main(int argc, char* argv[]) {
	ifstream input(argv[1], ios::in);

	if (!input)
	{
		cout << argv[1] << " could not be opened!\n";
		exit(3);
	}

	string infoPath, imagePath, maskPath, buf, bufImage, bufMask;
	double level1, width1, level2, width2, RS = 0, RI = 0;
	getline(input, infoPath, '\n');
	getline(input, imagePath, '\n');
	getline(input, maskPath, '\n');
	getline(input, buf, '\n'); level1 = stod(buf);
	getline(input, buf, '\n'); width1 = stod(buf);
	getline(input, buf, '\n'); level2 = stod(buf);
	getline(input, buf, '\n'); width2 = stod(buf);
	
	ifstream info(infoPath, ios::in);
	if (!info)
	{
		cout << infoPath << " could not be opened\n";
		exit(3);
	}
	while (getline(info, buf, '\n'))
	{
		if (!buf.compare(0, 9, "Rescale S"))
		{
			string str = buf.substr(buf.find(',') + 1, buf.size());
			RS = stod(str);
		}
		else if (!buf.compare(0, 9, "Rescale I"))
		{
			string str = buf.substr(buf.find(',') + 1, buf.size());
			RI = stod(str);
			break; /*The following info is not needed*/
		}
	}

	Mat image1(512, 512, CV_8U), image2(512, 512, CV_8U), 
		maskImg(512, 512, CV_8U), flax(512, 512, CV_8U), rotated;
	vector <double> ws, w2s;

	ifstream image(imagePath, ios::in); 
	if (!image)
	{
		cout << imagePath << " could not be opened\n";
		exit(3);
	}

	ifstream mask(maskPath, ios::in);
	vector <double> masks, flaxValue;
	if (!mask)
	{
		cout << maskPath << " colud not be opened\n";
		exit(3);
	}

	while (getline(image, bufImage, '\n'))
	{
		size_t pos;
		while ((pos = bufImage.find(',')) != string::npos)
		{
			string value = bufImage.substr(0, pos);
			bufImage.erase(0, pos + 1);
			double h = stod(value) * RS + RI;
			double w = (h <= level1 - width1 / 2) ? 0 :
				(h > level1 + width1 / 2) ? 255 :
				abs((255 * ((h - level1) / width1 + 0.5)));
			double w2 = (h <= level2 - width2 / 2) ? 0 :
				(h > level2 + width2 / 2) ? 255 :
				(255 * ((h - level2) / width2 + 0.5));
			ws.push_back(w); w2s.push_back(w2);
		}
		string value = bufImage; /*the last value of each row*/

		double h = stod(value) * RS + RI;
		double w = (h <= level1 - width1 / 2) ? 0 :
			(h > level1 + width1 / 2) ? 255 :
			abs((255 * ((h - level1) / width1 + 0.5)));
		double w2 = (h <= level2 - width2 / 2) ? 0 :
			(h > level2 + width2 / 2) ? 255 :
			(255 * ((h - level2) / width2 + 0.5));
		ws.push_back(w); w2s.push_back(w2);

		getline(mask, bufMask, '\n');
		while ((pos = bufMask.find(',')) != string::npos)
		{
			string value1 = bufMask.substr(0, pos);
			bufMask.erase(0, pos + 1);
			double maskValue = stod(value1);
			flaxValue.push_back(maskValue == 3 ? 255 : 0);
			masks.push_back(floor(abs(255.0 / 16 * maskValue)));
		}
		string value1 = bufMask;
		double maskValue = stod(value1);
		flaxValue.push_back(maskValue == 3 ? 255 : 0);
		masks.push_back(floor(abs(255.0 / 16 * maskValue)));
	}
	size_t count = 0;
	for (int i = 0; i < image1.rows; i++) /*put value in matrix*/
		for (int j = 0; j < image1.cols; j++)
			image1.at<uchar>(i, j) = floor(ws[count]),
			image2.at<uchar>(i, j) = floor(w2s[count]), /*level2 & width 2*/
			maskImg.at<uchar>(i, j) = masks[count], /*make mask image*/
			flax.at<uchar>(i, j) = flaxValue[count++]; /*make flax image*/

	imwrite(argv[2], image1);
	/*imshow("test window", image1);
	waitKey(0);
	imshow("windowing2", image2);
	waitKey(0);
	imshow("mask", maskImg);
	waitKey(0);
	imshow("flax", flax);
	waitKey(0);*/
	
	Mat channels[3] = { maskImg, image1, image2 };
	Mat after_merge;
	merge(channels, 3, after_merge);
	/*imshow("merge", after_merge);
	waitKey(0);*/

	vector<vector<Point>> contour;
	vector<Vec4i> hierarchy;
	Vec4d lines;
	findContours(flax, contour, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
	double maxArea = 0, maxArea2 = 0;
	int max = 0, max2 = 0;

	for (int i = 0; i < contour.size(); i++)
		if (contourArea(contour[i]) > maxArea)
			maxArea = contourArea(contour[i]), max = i;

	for (int i = 0; i < contour.size(); i++)
		if (contourArea(contour[i]) > maxArea2 && i != max)
			maxArea2 = contourArea(contour[i]), max2 = i;

	if(maxArea2 > maxArea * 0.3 || contour.size() == 2)
		contour[max].insert(contour[max].end(), contour[max2].begin(), contour[max2].end());

	fitLine(contour[max], lines, 2, 0, 0.000001, 0.000001);
	double lefty = (-lines[2] * lines[1] / lines[0]) + lines[3];
	double righty = ((512 - lines[2]) * lines[1] / lines[0]) + lines[3];
	double slope = 511 / (lefty - righty);
	/*line(flax, Point(flax.cols - 1, righty), Point(0, lefty), Scalar(128, 0, 0), 2);
	imshow("flax2", flax);
	waitKey(0);*/

	double angle = atan(slope) * 180 / CV_PI;
	/*cout << fixed << setprecision(6) << slope << " " << (angle) << " "
		<< (lefty - righty) << " " << -(flax.cols - 1) << endl;*/

	Mat rotation = getRotationMatrix2D(Point(256, 256), angle, 1.0);
	warpAffine(after_merge, rotated, rotation, after_merge.size());
	/*imshow("rot", rotated);
	waitKey(0);*/
	imwrite(argv[3], rotated);
	
	ofstream anglePath(argv[4], ios::out);
	if (!anglePath)
		cout << "fail to open " << argv[4], exit(3);

	anglePath << angle << endl;

	cout << (double)clock() << "MS";
 }