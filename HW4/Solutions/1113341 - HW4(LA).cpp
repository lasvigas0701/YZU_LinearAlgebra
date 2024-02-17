#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <iomanip>
using namespace std;
using namespace cv;

class Car
{
public:
	Car() {};
	Car(Mat image, Point2f rec[]) {
		diagonalPoints(rec);
		width = rightDown.x - leftUp.x;
		setCenter(rec);
		setHead(rec);
		Vec3b color = image.at<Vec3b>((int)center.y, (int)center.x);
		B = color[0];
		G = color[1];
		R = color[2];
		setColorID();
	}
	int getBlue() const {
		return this->B;
	}
	int getGreen() const {
		return this->G;
	}
	int getRed() const {
		return this->R;
	}
	Point getCenter() const {
		return this->center;
	}
	Point getHead() const {
		return this->head;
	}
	string getColorID() const {
		return this->colorID;
	}
	double getWidth() const {
		return this->width;
	}
	void diagonalPoints(Point2f rec[]) {
		Point2f leftUp = rec[0], rightDown = rec[0];
		for (int i = 1; i < 4; i++)
		{
			if (leftUp.x + leftUp.y > rec[i].x + rec[i].y) /*leftUp is the point which x+y is the smallest*/
				leftUp = rec[i];
			if (rightDown.x + rightDown.y < rec[i].x + rec[i].y) /*rightDown is x+y max*/
				rightDown = rec[i];
		}
		this->leftUp = leftUp, this->rightDown = rightDown;
	}
	
	/*find and set center of car*/
	void setCenter(Point2f rec[]) {
		this->center.x = (leftUp.x + rightDown.x) / 2;
		this->center.y = (leftUp.y + rightDown.y) / 2;
	}
	/*find and set head of car*/
	void setHead(Point2f rec[]) {
		this->head.x = (leftUp.x + rightDown.x) / 2;
		this->head.y = rightDown.y;
	}
	string tenToHex(int valueQuotient, int valueRemainder) {
		string tmp = "";
		tmp=char(valueQuotient > 9 ? 'A' - 10 + valueQuotient : '0' + valueQuotient);
		tmp+=char(valueRemainder > 9 ? 'A' - 10 + valueRemainder : '0' + valueRemainder);
		return tmp;
	}
	void setColorID() {
		string red, green, blue;
		red = tenToHex(R / 16, R % 16);
		green = tenToHex(G / 16, G % 16);
		blue = tenToHex(B / 16, B % 16);
		this->colorID = "#" + red + green + blue;
	}
	void setColorID(string newID) {
		this->colorID = newID;
	}
	void setBGR(int B, int G, int R) {
		this->B = B;
		this->G = G;
		this->R = R;
	}
private:
	int B;
	int G;
	int R;
	double width;
	Point2f leftUp;
	Point2f rightDown;
	Point center;
	Point head;
	string colorID;
};

struct Track {
	int B;
	int G;
	int R;
	Point center;
	Point head;
	double path = 0.0;
};

/*store odd frames*/
vector<Mat> videoResample(const vector<Mat>& clip) {
	vector<Mat> resampledClip; /*store frames which are resampled*/
	for (int i = 0; i < clip.size(); i += 2) /*traverse clip, then extract one in every two frames*/
		resampledClip.push_back(clip[i]);
	return resampledClip;
}

double computeDistance(Point2f a, Point2f b) {
	double diffX = a.x - b.x;
	double diffY = a.y - b.y;

	return (sqrt(diffX * diffX + diffY * diffY));
}

bool sameCar(Mat frame, Car& car, map<string, Track> cars, double& distance)
{
	for (auto x : cars)
	{
		double tmp = computeDistance(car.getHead(), x.second.head);

		if (abs(car.getBlue() - x.second.B) <= 5 &&
			abs(car.getGreen() - x.second.G) <= 5 &&
			abs(car.getRed() - x.second.R) <= 5 &&
			tmp <= car.getWidth())
		{
			if (car.getColorID() != x.first)
			{
				car.setColorID(x.first);
				car.setBGR(x.second.B, x.second.G, x.second.R);
			}
			distance = tmp;
			return true;
		}
	}
	return false;
}

/*store new car into cars*/
void newCar(map<string, Track>& cars, Car car)
{
	cars[car.getColorID()].center = car.getCenter();
	cars[car.getColorID()].head = car.getHead();
	cars[car.getColorID()].R = car.getRed();
	cars[car.getColorID()].G = car.getGreen();
	cars[car.getColorID()].B = car.getBlue();
}

int main(int argc, char** argv) {
	VideoCapture cap(argv[1]);
	Mat frame;
	vector<Mat> allFrames;
	while (cap.read(frame) && !frame.empty()) { /*read every frame*/
		Mat tmp;
		frame.copyTo(tmp);
		allFrames.push_back(tmp);
	}
	vector<Mat> resampledFrames = videoResample(allFrames);
	int videoHeight = resampledFrames[0].rows - 2;

	map<string, Track> cars;
	for (int i = 0; i < resampledFrames.size(); i++)
	{
		Mat greyImage, binaryImage;
		cvtColor(resampledFrames[i], greyImage, COLOR_BGR2GRAY);
		threshold(greyImage, binaryImage, 100, 255, THRESH_BINARY);

		/*vector<vector<Point>> cornerCandidate; needed in function drawContours for debug*/
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(binaryImage, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
		for (const auto& x : contours)
		{
			double distance = 0;
			if (contourArea(x) > 1000) /*is car*/
			{
				/*cornerCandidate.push_back(x);*/
				RotatedRect minRec = minAreaRect(x);
				Point2f points[4];
				minRec.points(points);
				Car car(resampledFrames[i], points);
				if (i > 0)
				{
					if (car.getHead().y >= videoHeight) /*touch bottom*/
						continue;

					if (sameCar(resampledFrames[i], car, cars, distance))
					{
						cars[car.getColorID()].center = car.getCenter();
						cars[car.getColorID()].head = car.getHead();
						cars[car.getColorID()].path += distance;
					}

					else /*new car*/
						newCar(cars, car);
				}
				else /*the first frame*/
					newCar(cars, car);
			}
		}
		
		//drawContours(resampledFrames[i], cornerCandidate, -1, Scalar(0, 255, 0), 2);
		//imshow("test", resampledFrames[i]); waitKey(0);
	}

	/*output*/
	fstream outFile(argv[2], ios::out);
	for (auto x : cars)
		outFile << x.first << ' ' << fixed << setprecision(2) << x.second.path << endl;
}