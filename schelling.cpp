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
		void setLocation(unsigned int location);
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

void Resident::setLocation(unsigned int location) {
	_location = location;
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
		int getNeighborhoodSize(unsigned int location);
		unsigned int placeResident(Resident* resident, unsigned int location);
		double computeSatisfaction(unsigned int location, unsigned int type);
		friend std::ostream& operator<< (std::ostream &out, const City &city);
		friend class Simulation;
};

int City::getNeighborhoodRadius() {
	return _nRadius;
}

int City::getNeighborhoodSize(unsigned int location) {
	if(location >= _width*_height) {
		std::cerr << "Invalid location\n";
		return -1;
	}

	int x = location%_width;
	int y = location/_width;
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
	return count;
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
	int count = getNeighborhoodSize(location);
	// int count = 2*(_nRadius+1)*_nRadius;
	//
	// int x_offset = (x-_nRadius) < (_width-x-1-_nRadius)?(x-_nRadius):(_width-x-1-_nRadius);
	// int y_offset = (y-_nRadius) < (_height-y-1-_nRadius)?(y-_nRadius):(_height-y-1-_nRadius);
	// int total_offset = x_offset+y_offset;
	//
	// if(x_offset < 0) {
	// 	count -= x_offset*x_offset;
	// }
	// if(y_offset < 0) {
	// 	count -= y_offset*y_offset;
	// }
	// if(-total_offset > (_nRadius+1)) {
	// 	total_offset = (-(_nRadius+1)-total_offset)*(-(_nRadius+1)-total_offset+1)/2;
	// 	count += total_offset;
	// }

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
		std::vector<unsigned int> _unsatisfied; //locations of unsatisfied residents
		std::vector<unsigned int> _openLocations;
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
		~Simulation() {
			if(_popBreakdown) delete[] _popBreakdown;
		}
		int placeResidents();
		int initializeSimulation(double threshold);
		int relocateResident(unsigned int currentLocation, unsigned int newLocation, double s);
		int iterate();
		friend std::ostream& operator<< (std::ostream &out, const Simulation &sim);
};

int Simulation::placeResidents() {

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

int Simulation::initializeSimulation(double threshold) {

	_threshold = threshold;
	std::random_device rd;
	std::mt19937 mt(rd());

	Resident* r = nullptr;
	//Place in vector by row major order for now
	for(unsigned int i = 0; i < (_city->_height)*(_city->_width); i++) {
		if((r = (_city->_map)[i])) {
			double s = _city->computeSatisfaction(i, r->getType());
			r->setSatisfaction(s);
			if(s < _threshold) {
				_unsatisfied.push_back(i);
			}
		} else {
			_openLocations.push_back(i);
		}
	}

	return 0;
}

int Simulation::iterate() {
	std::vector<unsigned int> tempUnsatisfied;
	std::vector<unsigned int> tempOpen;
	unsigned int currentLocation;

	//Longest unsatisfied resident and newest openLocation get priority

	// Iterating through each unsatisfied resident
	for(int i = 0; i < _unsatisfied.size(); i++) {
		currentLocation = _unsatisfied.at(i);
		if(_city->_map[currentLocation]->getSatisfaction() < _threshold) {
			//Finding location that will satisfy resident
			for(int j = 0; j < _openLocations.size(); j++) {
				unsigned int newLocation = _openLocations.at(j);
				if(_city->_map[newLocation]) {
					j++;
				} else {
					Resident* r = _city->_map[currentLocation];
					double s = _city->computeSatisfaction(newLocation, r->getType());
					if(s >= _threshold) {
						//Relocating resident
						r->setSatisfaction(s);
						r->setLocation(newLocation);
						_city->_map[newLocation] = r;
						_city->_map[currentLocation] = nullptr;

						//Updating old neighbors' satisfactions
						int x = currentLocation%(_city->_width);
						int y = currentLocation/(_city->_width);

						Resident *neighbor = nullptr;

						// Iterating through neighborhood
						int k = ((y-(_city->_nRadius)) > 0)?y-(_city->_nRadius):0;
						for(; k <= y+(_city->_nRadius) && k < (_city->_height); k++) {
							int l = ((x-(_city->_nRadius)+abs(y-k)) > 0? x-(_city->_nRadius)+abs(y-k):0);
							for(; l <= x+(_city->_nRadius)-abs(y-k) && l < (_city->_width); l++) {
								// Checking if there is a neighbor
								if((neighbor = _city->_map[k*(_city->_width)+l])) {
									// Recalculating neighbor's satisfaction
									int neighborNSize = _city->getNeighborhoodSize(k*(_city->_width)+l);
									double neighborUnscaledSatisfaction = neighbor->getSatisfaction() * neighborNSize;
									neighborUnscaledSatisfaction = neighborUnscaledSatisfaction + ((r->getType() == neighbor->getType())?-0.5:0.5);
									neighbor->setSatisfaction(neighborUnscaledSatisfaction/neighborNSize);

									// If neighbor is now unsatisfied, add to tempUnsatisfied
									if(neighbor->getSatisfaction() < _threshold) {
										tempUnsatisfied.push_back(neighbor->getLocation());
									}
								}
								if((k == y) && (k == (x-1))) k++;
							}
						}

						// Updating new neighbors' satisfaction
						x = newLocation%(_city->_width);
						y = newLocation/(_city->_width);

						neighbor = nullptr;
						// Iterating through neighborhood
						k = ((y-(_city->_nRadius)) > 0)?y-(_city->_nRadius):0;
						for(; k <= y+(_city->_nRadius) && k < (_city->_height); k++) {
							int l = ((x-(_city->_nRadius)+abs(y-k)) > 0? x-(_city->_nRadius)+abs(y-k):0);
							for(; l <= x+(_city->_nRadius)-abs(y-k) && l < (_city->_width); l++) {
								// Checking if there is a neighbor
								if((neighbor = _city->_map[k*(_city->_width)+l])) {
									// Recalculating neighbor's satisfaction
									int neighborNSize = _city->getNeighborhoodSize(k*(_city->_width)+l);
									double neighborUnscaledSatisfaction = neighbor->getSatisfaction() * neighborNSize;
									neighborUnscaledSatisfaction = neighborUnscaledSatisfaction + ((r->getType() == neighbor->getType())?0.5:-0.5);
									neighbor->setSatisfaction(neighborUnscaledSatisfaction/neighborNSize);

									// If neighbor is now unsatisfied, add to tempUnsatisfied
									if(neighbor->getSatisfaction() < _threshold) {
										tempUnsatisfied.push_back(neighbor->getLocation());
									}
								}
								if((k == y) && (k == (x-1))) k++;
							}
						}

						tempOpen.push_back(currentLocation);
						_unsatisfied[i] = newLocation;
						break;
					}
				}
			}
			//No location will satisfy the resident, so we keep them on the list
		} else {
			i++;
		}
	}

	// Resetting _openLocation list, newest open locations come first
	for(int i = 0; i < _openLocations.size(); i++) {
		// Check if still open
		if(_city->_map[_openLocations.at(i)] == nullptr) {
			tempOpen.push_back(_openLocations.at(i));
		}
	}
	_openLocations = tempOpen;

	// Resetting _unsatisfied list, longest unsatisfied resident comes first
	int newlySatisfiedCount = 0;
	for(int i = 0; i < _unsatisfied.size(); i++) {
		Resident* r = _city->_map[_unsatisfied.at(i)];
		if(r == nullptr) {
			std::cerr << "ERROR: Resident on _unsatisfied list should not be nullptr\n";
		}
		if(r->getSatisfaction() < _threshold) {
			_unsatisfied[i-newlySatisfiedCount] = _unsatisfied[i];
		} else {
			newlySatisfiedCount++;
		}
	}
	for(int i = 0; i < newlySatisfiedCount; i++) {
		_unsatisfied.pop_back();
	}
	for(int i = 0; i < tempUnsatisfied.size(); i++) {
		_unsatisfied.push_back(tempUnsatisfied[i]);
	}

	// Return 0 on success;
	return 0;
}

std::ostream& operator<< (std::ostream &out, const Simulation &sim) {
	out << "Iteration: " << sim._iteration << "\n" << *(sim._city) << "Threshold: " << sim._threshold << " Neighborhood radius: " << sim._city->getNeighborhoodRadius() << "\nUnsatisfied residents: " << sim._unsatisfied.size() << " Open Locations: " << sim._openLocations.size();
	return out;
}

int main(int argc, char* argv[]) {

	double popBreakdown[] = {0.5, 0.5};
	Simulation sim = Simulation(30, 30, 2, 800, 2, popBreakdown);
	sim.placeResidents();
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
