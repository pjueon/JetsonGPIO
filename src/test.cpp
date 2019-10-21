#include <iostream>
#include "JetsonGPIO.h"

using namespace std;

int main(){
	cout << "model: "<< GPIO::model << endl;
	cout << "lib version: " << GPIO::VERSION << endl;
	cout << "hello Jetson" << endl;
	
	return 0;
}
