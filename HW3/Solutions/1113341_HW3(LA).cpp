#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <algorithm>
using namespace std;
using namespace cv;

const double minArea = 100, maxArea = 1000;

int main(int argc, char* argv[])
{
	Mat image = imread(argv[1], 0); /*read the input image with gray scale in order to binarization*/
	Mat binary;
	threshold(image, binary, 80, 255, THRESH_BINARY); /*Binarization*/

	Mat remainContour(image.rows, image.cols, THRESH_BINARY);
	vector<vector<Point>> contours, cornerCandidate;
	vector<Vec4i> hierarchy;
	findContours(binary, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
	for (const auto& x : contours)
	{
		double area = contourArea(x);
		Rect rectangle = boundingRect(x);
		double ratio = double(rectangle.width) / rectangle.height;
		if (area >= minArea && area <= maxArea && ratio >= 0.5 && ratio <= 1.5) 
			cornerCandidate.push_back(x);
	}
	drawContours(remainContour, cornerCandidate, -1, 255, 2);

	/*set 4 corners for perspective*/
	Point2f cornerPoint[4], corners[4]{};/*sequence: left-up, right-up, right-down, left-down*/
	cornerPoint[0] = Point2f(0, 0); /*x+y min*/
	cornerPoint[1] = Point2f(remainContour.cols - 1, 0); /*x-y max*/
	cornerPoint[2] = Point2f(remainContour.cols - 1, remainContour.rows - 1); /*x+y max*/
	cornerPoint[3] = Point2f(0, remainContour.rows - 1); /*x - y min*/
	double xPlusY_max = 0, xMinY_max = 0, xPlusY_min = 1500, xMinY_min = 1500;
	for (int i = 0; i < remainContour.rows; i++)
	{
		for (int j = 0; j < remainContour.cols; j++)
		{
			if (remainContour.ptr<uchar>(i)[j] == 255)
			{
				if (xPlusY_min > j + i) corners[0] = Point2f(j, i), xPlusY_min = j + i;
				else if (xMinY_max < j - i) corners[1] = Point2f(j, i), xMinY_max = j - i;
				else if (xPlusY_max < j + i) corners[2] = Point2f(j, i), xPlusY_max = j + i;
				else if (xMinY_min > j - i) corners[3] = Point2f(j, i), xMinY_min = j - i;
			}
		}
	}

	Size A4(210 * 5, 297 * 5);
	Mat perspective = getPerspectiveTransform(corners, cornerPoint);
	Mat transformedImage;
	warpPerspective(image, transformedImage, perspective, A4);

	/*cut answer sheet*/
	int x = 230, y = 515, width = 760, height = 800;
	Rect rec(x, y, width, height);
	Mat answerSheet = transformedImage(rec);
	Mat answerSheetBinary;
	threshold(answerSheet, answerSheetBinary, 120, 255, THRESH_BINARY);

	/*cut height location point*/
	x = 70, width = 70;
	Rect rec_2(x, y, width, height);
	Mat heightRecord = transformedImage(rec_2);
	Mat heightRecordBinary;
	threshold(heightRecord, heightRecordBinary, 120, 255, THRESH_BINARY);

	/*find the contours of answer sheet*/
	vector<vector<Point>> contours_sheet;
	vector<Vec4i> hierarchy_sheet;
	findContours(answerSheetBinary, contours_sheet, hierarchy_sheet, RETR_TREE, CHAIN_APPROX_SIMPLE);
	double len;
	vector<vector<Point>> filled;

	/*determine which box is filled and record them by "filled", then remain them*/
	for (const auto& x : contours_sheet)
	{
		double area = contourArea(x);
		len = arcLength(x, 1);
		RotatedRect minRec = minAreaRect(x); /*minimum bounding rectangle to find the left up corner*/
		Point2f point[4], leftUp;
		minRec.points(point);
		leftUp = point[0]; /*center of minArea*/
		for (int i = 1; i < 4; i++)
		{
			if (point[i].x < leftUp.x) 
				leftUp.x = point[i].x;
			if (point[i].y < leftUp.y) 
				leftUp.y = point[i].y;
		}
		if (area >= 150 && area < 350 && len <= 100)
			if (!answerSheetBinary.at<uchar>(leftUp.y + 5, leftUp.x + 10))
				filled.push_back(x);
	}
	Mat fillAnswer(answerSheetBinary.rows, answerSheetBinary.cols, THRESH_BINARY);
	drawContours(fillAnswer, filled, -1, 255, -1);

	vector<vector<Point>> contoursHeight;
	vector<Vec4i> hierarchyHeight;
	findContours(heightRecordBinary, contoursHeight, hierarchyHeight, RETR_TREE, CHAIN_APPROX_SIMPLE);
	vector<int> heightLocation;
	for (const auto& x : contoursHeight)
	{
		double area = contourArea(x);
		if (area >= minArea && area <= maxArea)
		{
			RotatedRect miniRec = minAreaRect(x);
			Point2f cornerPoints[4];
			double leftUp, rightDown;
			miniRec.points(cornerPoints);
			leftUp = rightDown = cornerPoints[0].y;
			for (int i = 1; i < 4; i++)
			{
				if (cornerPoints[i].y < leftUp)
					leftUp = cornerPoints[i].y;
				if (cornerPoints[i].y > rightDown)
					rightDown = cornerPoints[i].y;
			}
			heightLocation.push_back((leftUp + rightDown) / 2); /*the height of center of miniRec*/
		}
	}

	/*record output answer*/
	int times[24]{};
	int output[24]{};
	for (int i = 0; i < 24; i++)
	{
		for (int j = 0, x = 45; j < 12; j++, x+=61)
		{
			if (fillAnswer.ptr(heightLocation[i])[x] > 250)
			{ 
				times[i]++;
				if (j >= 0 && j < 9)
					output[i] = j + 1;
				else if (j == 9)
					output[i] = 0;
				else
					output[i] = j;
			}
		}
	}

	/*output*/
	fstream outFile(argv[2], ios::out);
	for (int i = 23; i >= 0; i--)
	{
		if (!times[i])
			outFile << 'X';
		else if (times[i] == 1)
		{
			if (output[i] == 10)
				outFile << 'A';
			else if (output[i] == 11)
				outFile << 'B';
			else
				outFile << output[i];
		}
		else
			outFile << 'M';
	}
	outFile << endl;
}