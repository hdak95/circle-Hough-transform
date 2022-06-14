#include <iostream>
#include <fstream>
#include <cmath>
#include<algorithm>
#include<vector>
#include<tuple>
using namespace std;

#define width 396	// input image width
#define height 400	// input image height
#define pi 3.141592
#define rmax 200	// radius maximum

int gaussian[25] = { 2,4,5,4,2,4,9,12,9,4,5,12,15,12,5,4,9,12,9,4,2,4,5,4,2 };// 5x5 filter ¥ò=1
int xSobel[9] = { -1,0,1,-2,0,2,-1,0,1 };	// x axis sobel filter
int ySobel[9] = { 1,2,1,0,0,0,-1,-2,-1 };	// y axis sobel filter
int high = 200;								// high threshold
int low = 100;								// low threshold
int voteThreshold = 100;					// vote threshold

void padding(unsigned char* img, unsigned char* pad,int size);
void zeropadding(unsigned char* img, unsigned char* pad);
void gaussianblur(unsigned char* img, unsigned char* pad);
void sobelfilter(unsigned char*img,unsigned char* pad,double*angle);
void NMS(unsigned char* img, unsigned char* pad, double* angle);
void doubleThresholding(unsigned char* img);
void edgeTracking(unsigned char* img, unsigned char* pad);
void houghTransform(unsigned char* img, unsigned char* vote, vector<vector<int>>&circle);
void drawCircle(unsigned char* img, vector<vector<int>>&circle);

int main() {
	//image file read
	ifstream fInput;
	fInput.open("Test_img_CV_HW4_396x400.yuv", ios::binary);
	unsigned char* buffer = new unsigned char[width * height];
	fInput.read((char*)buffer, width * height);
	fInput.close();
	
	//5x5 gaussian filter
	unsigned char* gaussianPaddingBuffer = new unsigned char[(width + 4) * (height + 4)];
	padding(buffer, gaussianPaddingBuffer, 5);
	gaussianblur(buffer, gaussianPaddingBuffer);	
	delete[] gaussianPaddingBuffer;
	
	//save gaussian filter image
	unsigned char* paddingBuffer = new unsigned char[(width + 2) * (height + 2)];
	ofstream fOutput;
	fOutput.open("Test_img_CV_HW4_gaussian_396x400.yuv", ios::binary);
	fOutput.write((char*)buffer, width * height);
	fOutput.close();

	//sobel filter
	double* angleBuffer = new double[width * height];
	padding(buffer, paddingBuffer, 3);
	sobelfilter(buffer, paddingBuffer, angleBuffer);

	//save sobel filter image
	fOutput.open("Test_img_CV_HW4_gradient_396x400.yuv", ios::binary);
	fOutput.write((char*)buffer, width * height);
	fOutput.close();
	
	//Non-maximum suppression
	zeropadding(buffer, paddingBuffer);
	NMS(buffer, paddingBuffer, angleBuffer);
	
	//save Non-maximum suppression image
	fOutput.open("Test_img_CV_HW4_NMS_396x400.yuv", ios::binary);
	fOutput.write((char*)buffer, width * height);
	fOutput.close();

	//Double Thresholding
	doubleThresholding(buffer);

	//save Double Thresholding image
	fOutput.open("Test_img_CV_HW4_doubleThreshold_396x400.yuv", ios::binary);
	fOutput.write((char*)buffer, width * height);
	fOutput.close();

	//Hysteresis edge tracking
	zeropadding(buffer, paddingBuffer);
	edgeTracking(buffer, paddingBuffer);
	
	//save canny edge detector applied image
	fOutput.open("Test_img_CV_HW4_edgeTracking_396x400.yuv", ios::binary);
	fOutput.write((char*)buffer, width * height);
	fOutput.close();

	//circle hough transform
	unsigned char* vote = new unsigned char[(width + 2 * rmax) * (height + 2 * rmax) * 70];
	for (int i = 0; i < (width + 2 * rmax) * (height + 2 * rmax) * 70; i++) {
		vote[i] = 0;
	}
	vector<vector<int>>circle;
	houghTransform(buffer, vote,circle);
	
	//read original image
	fInput.open("Test_img_CV_HW4_396x400.yuv", ios::binary);
	fInput.read((char*)buffer, width * height);
	fInput.close();
	
	//draw detected circles at original image
	drawCircle(buffer, circle);

	//save circle detected image
	fOutput.open("Test_img_CV_HW4_CircleDetection_396x400.yuv", ios::binary);
	fOutput.write((char*)buffer, width * height);
	fOutput.close();

	delete[] buffer;
	delete[] paddingBuffer;
	delete[] angleBuffer;
	delete[] vote;
}

//same padding
void padding(unsigned char* img, unsigned char* pad, int size) {
	//upper copy padding
	for (int i = 0; i < size / 2; i++) {
		int j = 0;
		for (j; j < size / 2; j++) {
			pad[i * (width + size - 1) + j] = img[0];
		}
		for (j; j < size / 2 + width; j++) {
			pad[i * (width + size - 1) + j] = img[j - size / 2];
		}
		for (j; j < size + width - 1; j++) {
			pad[i * (width + size - 1) + j] = img[width - 1];
		}
	}
	//left right copy padding
	for (int i = size / 2; i < size / 2 + height; i++) {
		//left
		for (int j = 0; j < size / 2; j++) {
			pad[i * (width + size - 1) + j] = img[(i - size / 2) * width];
		}
		for (int j = size / 2; j < size / 2 + width; j++) {
			pad[i * (width + size - 1) + j] = img[(i - size / 2) * width + j - size / 2];
		}
		//right
		for (int j = size / 2 + width; j < size + width - 1; j++) {
			pad[i * (width + size - 1) + j] = img[(i - size / 2 + 1) * width - 1];
		}
	}
	//down copy padding
	for (int i = size / 2 + height; i < size + height - 1; i++) {
		int j = 0;
		for (j; j < size / 2; j++) {
			pad[i * (width + size - 1) + j] = img[(height - 1) * width];
		}
		for (j; j < size / 2 + width; j++) {
			pad[i * (width + size - 1) + j] = img[(height - 1) * width + j - size / 2];
		}
		for (j; j < size + width - 1; j++) {
			pad[i * (width + size - 1) + j] = img[height * width - 1];
		}
	}
}

//zero padding
void zeropadding(unsigned char* img, unsigned char* pad) {
	for (int i = 0; i < height + 2; i++) {
		for (int j = 0; j < width + 2; j++) {
			pad[i * (width + 2) + j] = 0;
		}
	}
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			pad[(i + 1) * (width + 2) + j + 1] = img[i * width + j];
		}
	}
}

//gaussian blur
//5*5 filter
void gaussianblur(unsigned char* img, unsigned char* pad) {
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int sum = 0;
			for (int k = 0; k < 5; k++) {
				for (int l = 0; l < 5; l++) {
					sum += pad[(i + k) * (width + 4) + j + l] * gaussian[k * 5 + l];
				}
			}
			img[i * width + j] = sum / 159;
		}
	}
}

//sobel filter
//gradient filter
void sobelfilter(unsigned char* img, unsigned char* pad, double* angle) {
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int xSum = 0;
			int ySum = 0;
			//x,y axis gradient
			for (int k = 0; k < 3; k++) {
				for (int l = 0; l < 3; l++) {
					xSum += pad[(i + k) * (width + 2) + j + l] * xSobel[k * 3 + l];
					ySum += pad[(i + k) * (width + 2) + j + l] * ySobel[k * 3 + l];
				}
			}
			img[i * width + j] = abs(xSum) + abs(ySum);
			//edge gradient
			double temp = atan2(ySum, xSum) * 180 / pi;
			if (temp < 0)
				temp += 180;
			angle[i * width + j] = temp;
		}
	}
}

//Non-maximum suppression
//edge suppression
void NMS(unsigned char* img, unsigned char* pad, double* angle) {
	int left = 0, right = 0;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			//direction to compare
			if ((0 <= angle[i * width + j] < 22.5) || (157.5 <= angle[i * width + j] <= 180)) {
				left = pad[(i + 1) * (width + 2) + j];
				right = pad[(i + 1) * (width + 2) + j + 2];
			}
			else if (22.5 <= angle[i * width + j] < 67.5) {
				left = pad[i * (width + 2) + j + 2];
				right = pad[(i + 2) * (width + 2) + j];
			}
			else if (67.5 <= angle[i * width + j] < 112.5) {
				left = pad[i * (width + 2) + j + 1];
				right = pad[(i + 2) * (width + 2) + j + 1];
			}
			else if (112.5 <= angle[i * width + j] < 157.5) {
				left = pad[i * (width + 2) + j];
				right = pad[(i + 2) * (width + 2) + j + 2];
			}
			//suppression
			if (pad[(i + 1) * (width + 2) + j + 1] >= left && pad[(i + 1) * (width + 2) + j + 1] >= right)
				img[i * width + j] = pad[(i + 1) * (width + 2) + j + 1];
			else
				img[i * width + j] = 0;
		}
	}
}

//double Thresholding
//detect strong edge,weak edge
void doubleThresholding(unsigned char* img) {
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			if (img[i * width + j] >= high)
				img[i * width + j] = 255;
			else { 
				if (img[i * width + j] >= low) {
					img[i * width + j] = 80;
				}
				else
					img[i * width + j] = 0;
			}
		}
	}
}

//edge Tracking
//hysteresis edge tracking
//save weak edge near strong edge
//canny edge detector result
void edgeTracking(unsigned char* img, unsigned char* pad) {
	bool check;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			if (pad[(i + 1) * (width + 2) + j + 1] == 80) {//weak edge
				check = false;
				for (int k = -1; k < 2; k++) {
					for (int l = -1; l < 2; l++) {
						if (pad[(i + 1 + k) * (width + 2) + j + 1 + l] == 255)
							check = true;
					}
				}
				if (check == true)//strong edge exists nearby
					img[i * width + j] = 255;
				else
					img[i * width + j] = 0;
			}
		}
	}
}

//Hough Transform
//circle Hough Transform
void houghTransform(unsigned char* img, unsigned char* vote, vector<vector<int>>& circle) {
	//transform to a,b,r axis and vote
	for (int r = 30; r < 100; r++) {//find radius from 30~99
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				if (img[i * width + j] > 0) {
					for (int degree = 0; degree < 360; degree ++) {
						int aTemp = round(j - r * cos(pi * degree / 180));
						int bTemp = round(i - r * sin(pi * degree / 180));
						vote[(r - 30) * (width + 2 * rmax) * (height + 2 * rmax) + (bTemp + rmax) * (width + 2 * rmax) + aTemp + rmax]++;
					}
				}
			}
		}
	}
	vector < vector<int>>candidate;
	
	//save circle candidates more than threshold
	for (int r = 0; r < 70; r++) {
		for (int i = 0; i < height + 2 * rmax; i++) {
			for (int j = 0; j < width + 2 * rmax; j++) {
				if (vote[r * (height + 2 * rmax) * (width + 2 * rmax) + i * (width + 2 * rmax) + j] < voteThreshold)
					vote[r * (height + 2 * rmax) * (width + 2 * rmax) + i * (width + 2 * rmax) + j] = 0;
				else {
					vector<int>temp;
					temp.push_back(j - rmax);
					temp.push_back(i - rmax);
					temp.push_back(r + 30);
					temp.push_back(vote[r * (height + 2 * rmax) * (width + 2 * rmax) + i * (width + 2 * rmax) + j]);
					candidate.push_back(temp);
				}
			}
		}
	}

	//get one circle from each point
	circle.push_back(candidate[0]);
	for (int i = 1; i < candidate.size(); i++) {
		int flag = 0;
		for (int j = 0; j < circle.size(); j++) {
			//same circle center point 
			if (sqrt(pow(candidate[i][0] - circle[j][0], 2) + pow(candidate[i][1] - circle[j][1], 2)) < 30) {
				flag = 1;
				if (circle[j][3] < candidate[i][3]) {//more voted circle
					circle[j][0] = candidate[i][0];
					circle[j][1] = candidate[i][1];
					circle[j][2] = candidate[i][2];
					circle[j][3] = candidate[i][3];
				}
			}
		}
		//another circle
		if (flag == 0) {
			circle.push_back(candidate[i]);
		}
	}
	//Ground Truth
	for (int i = 0; i < circle.size(); i++) {
		cout << "(a,b) = ("<<circle[i][0] << ", " << circle[i][1] << "), r = " << circle[i][2] << endl;
	}
}

//draw circle at original image
void drawCircle(unsigned char* img, vector<vector<int>>& circle) {
	for (int i = 0; i < circle.size(); i++) {
		for (int j = 0; j < 720; j++) {
			int x = circle[i][0] + circle[i][2] * cos(pi * j / 360);
			int y = circle[i][1] + circle[i][2] * sin(pi * j / 360);
			img[y * width + x] = 255;
		}
	}
}