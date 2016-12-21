#include <iostream>
#include <cmath>
#include <vector>
#include <string.h>
#include <random>

class Resident {
	private:
		unsigned int _location;
		double _satisfaction;
		unsigned int _type;
	public:
		Resident(unsigned int type, unsigned int location);
		unsigned int getLocation();
		void setSatisfaction(double satisfaction);
		double getSatisfaction();
		unsigned int getType();
	friend class City;
};

Resident::Resident(unsigned int type, unsigned int location) {
	_type = type;
	_location = location;
	_satisfaction = 0;
}

unsigned int Resident::getLocation() {
	return _location;
}

void Resident::setSatisfaction(double satisfaction) {
	_satisfaction = satisfaction;
}

double Resident::getSatisfaction() {
	return _satisfaction;
}

unsigned int Resident::getType() {
	return _type;
}

class City {
	private:
		Resident** _map;
		int _height;
		int _width;
		int _nRadius;
	public:
		City(int h, int w, int nRadius) {
			_height = h;
			_width = w;
			_nRadius = nRadius;
			_map = new Resident*[h*w];
		}
		~City() {
			if(_map) delete[] _map;
		}
		int getNeighborhoodRadius();
		unsigned int placeResident(Resident* resident, unsigned int location);
		double computeSatisfaction(unsigned int location, unsigned int type);
		friend std::ostream& operator<< (std::ostream &out, const City &city);
		friend class Simulation;
};

int City::getNeighborhoodRadius() {
	return _nRadius;
}

unsigned int City::placeResident(Resident* resident, unsigned int location) {
	if(resident) {
		_map[location] = resident;
		return location;
	} else {
		std::cerr << "Resident is nullptr\n";
		return _height*_width+1;
	}
}

double City::computeSatisfaction(unsigned int location, unsigned int type) {
	if(location >= _width*_height) {
		std::cerr << "Invalid location\n";
		return -1;
	}

	int x = location%_width;
	int y = location/_width;
	double satisfaction = 0;
	int count = 2*(_nRadius+1)*_nRadius;

	int x_offset = (x-_nRadius) < (_width-x-1-_nRadius)?(x-_nRadius):(_width-x-1-_nRadius);
	int y_offset = (y-_nRadius) < (_height-y-1-_nRadius)?(y-_nRadius):(_height-y-1-_nRadius);
	int total_offset = x_offset+y_offset;

	if(x_offset < 0) {
		count -= x_offset*x_offset;
	}
	if(y_offset < 0) {
		count -= y_offset*y_offset;
	}
	if(-total_offset > (_nRadius+1)) {
		total_offset = (-(_nRadius+1)-total_offset)*(-(_nRadius+1)-total_offset+1)/2;
		count += total_offset;
	}

	Resident *r = nullptr;

	int i = ((y-_nRadius) > 0)?y-_nRadius:0;
	for(; i <= y+_nRadius && i < _height; i++) {
		int j = ((x-_nRadius+abs(y-i)) > 0?x-_nRadius+abs(y-i):0);
		for(; j <= x+_nRadius-abs(y-i) && j < _width; j++) {
			if((r = _map[i*_width+j])) {
				satisfaction += (type == r->_type)?1:0;
			} else {
				satisfaction += 0.5;
			}
			if((i == y) && (j == (x-1))) j++;
		}
	}

	return satisfaction /= count;
}

std::ostream& operator<< (std::ostream &out, const City &city)
{
	for(int i = 0; i < (city._height)*(city._width); i++) {
		if((city._map)[i]) {
			out << (city._map)[i]->getType() << " ";
		} else {
			out << "X ";
		}
		if((i%city._width) == (city._width-1)) out << "\n";
	}
  return out;
}

class Simulation {
	private:
		City* _city;
		unsigned int _population;
		unsigned int _numTypes;
		unsigned int* _popBreakdown;
		double _threshold;
		unsigned int _iteration;
		std::vector<Resident*> _unsatisfied;
		std::vector<int> _openLocations;
	public:
		Simulation(int h, int w, int nRadius, unsigned int pop, unsigned int numTypes, double* popBreakdown) {
			_city = new City(h, w, nRadius);
			_population = pop;
			_numTypes = numTypes;
			_iteration = 0;
			_popBreakdown = new unsigned int[_numTypes];
			for(int i = 0; i < _numTypes; i++) {
				_popBreakdown[i] = (unsigned int)round(popBreakdown[i]*_population);
			}
		};
		int initializeSimulation(double threshold);
		friend std::ostream& operator<< (std::ostream &out, const Simulation &sim);
};

int Simulation::initializeSimulation(double threshold) {
	_threshold = threshold;

	// Error checking
	if(_population >= (_city->_height)*(_city->_width)) {
		std::cerr << "ERROR: population greater than or equal to city capacity\n";
		return 1;
	}

	int counts[_numTypes+1];
	int sum = 0;
	for(int i = 0; i < _numTypes; i++) {
		counts[i] = _popBreakdown[i];
		sum += _popBreakdown[i];
	}
	counts[_numTypes] = (_city->_height)*(_city->_width)-_population;

	if(sum != _population) {
		std::cerr << "ERROR: population does not equal sum of subpopulations\n";
		return 1;
	}

	std::random_device rd;
	std::mt19937 mt(rd());

	// Randomly fill city locations
	for(int i = 0; i < _city->_height; i++) {
		for(int j = 0; j < _city->_width; j++) {
			unsigned int k = (mt()%(_numTypes+1));
			while(counts[k] == 0) {
				k = (mt()%(_numTypes+1));
			}
			counts[k]--;
			unsigned int location = (unsigned int)i*(_city->_width) + j;
			if(k == _numTypes) {
				(_city->_map)[location] = nullptr;
			} else {
				(_city->_map)[location] = new Resident(k, location);
			}
		}
	}
	return 0;
}

std::ostream& operator<< (std::ostream &out, const Simulation &sim) {
	out << *(sim._city) << "Iteration: " << sim._iteration;
	return out;
}

int main(int argc, char* argv[]) {

	double popBreakdown[] = {0.5, 0.5};
	Simulation sim = Simulation(30, 30, 2, 800, 2, popBreakdown);
	sim.initializeSimulation(0.5);

	std::cout << sim << std::endl;

	// int* city = new int[1000];
	//
	// memset (city,0,4000);
	//
	// int _width = 25;
	// int _height = 40;
	//
	// int _nRadius = 3;
	// unsigned int location = 899;
	//
	// int x = location%_width;
	// int y = location/_width;
	//
	// int i = ((y-_nRadius) > 0)?y-_nRadius:0;
	// for(; i <= y+_nRadius && i < _height; i++) {
	// 	int j = ((x-_nRadius+abs(y-i)) > 0?x-_nRadius+abs(y-i):0);
	// 	for(; j <= x+_nRadius-abs(y-i) && j < _width; j++) {
	// 		std::cout << j << " ";
	// 		city[i*_width+j] = 1;
	// 	}
	// }
	//
	// std::cout << std::endl;
	//
	// for(int i = 0; i < 1000; i++) {
	// 	std::cout << city[i] << " ";
	// 	if(i%_width == _width-1) std::cout << std::endl;
	// }
	//
	// int count = 2*(_nRadius+1)*_nRadius;
	//
	// int x_offset = (x-_nRadius) < (_width-x-1-_nRadius)?(x-_nRadius):(_width-x-1-_nRadius);
	// int y_offset = (y-_nRadius) < (_height-y-1-_nRadius)?(y-_nRadius):(_height-y-1-_nRadius);
	// int total_offset = x_offset+y_offset;
	//
	// // std::cout << "Initial count: " << count
	// // << " x_offset: " << x_offset << " y_offset: " << y_offset
	// // << " total_offset: " << -total_offset
	// // << " (_nRadius+1): " << (_nRadius+1) << std::endl;
	//
	// if(x_offset < 0) {
	// 	count -= x_offset*x_offset;
	// }
	// if(y_offset < 0) {
	// 	count -= y_offset*y_offset;
	// }
	// if(-total_offset > (_nRadius+1)) {
	// 	total_offset = (-(_nRadius+1)-total_offset)*(-(_nRadius+1)-total_offset+1)/2;
	// 	std::cout << total_offset << std::endl;
	// 	count += total_offset;
	// }
	//
	// std::cout << "Neighborhood size: " << count << std::endl;
}
