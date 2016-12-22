#include <iostream>
#include <cmath>
#include <vector>
#include <list>
#include <random>

class Resident {
	private:
		unsigned int _location;
		double _satisfaction;
		bool _isSatisfied;
		unsigned int _type;
	public:
		Resident(unsigned int type, unsigned int location);
		void setLocation(unsigned int location);
		unsigned int getLocation();
		void setSatisfaction(double satisfaction, double threshold);
		double getSatisfaction();
		bool isSatisfied();
		unsigned int getType();
	friend class City;
};

Resident::Resident(unsigned int type, unsigned int location) {
	_type = type;
	_location = location;
	_satisfaction = 0;
	_isSatisfied = true;
}

void Resident::setLocation(unsigned int location) {
	_location = location;
}

unsigned int Resident::getLocation() {
	return _location;
}

void Resident::setSatisfaction(double satisfaction, double threshold) {
	_satisfaction = satisfaction;
	_isSatisfied = _satisfaction >= threshold;
}

double Resident::getSatisfaction() {
	return _satisfaction;
}

bool Resident::isSatisfied() {
	return _isSatisfied;
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
		return (unsigned int)(_height*_width+1);
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
        Resident *r;
		if((r = (city._map)[i])) {
			out << (r->getType()) << ((r->isSatisfied())?" ":"U");
//            out << (r->getType()) << " ";
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
		std::list<unsigned int> _unsatisfied; //locations of unsatisfied residents
		std::list<unsigned int> _openLocations;
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
		int iterate();
        unsigned int getIteration();
        unsigned int numUnsatisfied();
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
    unsigned int seed = rd();
    std::cout << seed << std::endl;
	std::mt19937 mt(seed);

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

	Resident* r = nullptr;
	//Place in list by row major order for now
	for(unsigned int i = 0; i < (_city->_height)*(_city->_width); i++) {
		if((r = (_city->_map)[i])) {
			double s = _city->computeSatisfaction(i, r->getType());
			r->setSatisfaction(s, _threshold);
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
	unsigned int currentLocation;

	//Longest unsatisfied resident and newest openLocation get priority

    std::list<unsigned int> unsatisfiedNeighbors;

	// Iterating through each unsatisfied resident
    std::list<unsigned int>::iterator it_u = _unsatisfied.begin();
    std::list<unsigned int>::iterator it_uend = _unsatisfied.end();
    std::list<unsigned int>::iterator it_o;
    std::list<unsigned int>::iterator it_oend;

	for(it_u; it_u != it_uend; it_u++) {
		currentLocation = *it_u;
        if(_city->_map[currentLocation] == nullptr) {
            it_u = _unsatisfied.erase(it_u);
            it_u--;

        } else if(!(_city->_map[currentLocation]->isSatisfied())) {
			//Finding location that will satisfy resident

            it_oend = _openLocations.end();
			for(it_o = _openLocations.begin(); it_o != it_oend; it_o++) {
				unsigned int newLocation = *it_o;
				if(_city->_map[newLocation]) {
                    it_o = _openLocations.erase(it_o);
                    it_o--;
				} else {
					Resident* r = _city->_map[currentLocation];
					double s = _city->computeSatisfaction(newLocation, r->getType());
					if(s >= _threshold) {
						//Relocating resident
						r->setSatisfaction(s, _threshold);
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
                                    // Making sure neighbor is not relocated resident
                                    if(neighbor->getLocation() != r->getLocation()) {
                                        // Recalculating neighbor's satisfaction
                                        int neighborNSize = _city->getNeighborhoodSize(k * (_city->_width) + l);
                                        bool wasSatisfied = neighbor->isSatisfied();
                                        double neighborUnscaledSatisfaction =
                                                neighbor->getSatisfaction() * neighborNSize;
                                        neighborUnscaledSatisfaction = neighborUnscaledSatisfaction +
                                                                       ((r->getType() == neighbor->getType()) ? -0.5
                                                                                                              : 0.5);
                                        neighbor->setSatisfaction(neighborUnscaledSatisfaction / neighborNSize,
                                                                  _threshold);

                                        // If neighbor is newly unsatisfied, add to _unsatisfied
                                        if (!(neighbor->isSatisfied()) && wasSatisfied) {
                                            unsatisfiedNeighbors.push_back(neighbor->getLocation());
                                        }
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
                                    bool wasSatisfied = neighbor->isSatisfied();
									double neighborUnscaledSatisfaction = neighbor->getSatisfaction() * neighborNSize;
									neighborUnscaledSatisfaction = neighborUnscaledSatisfaction + ((r->getType() == neighbor->getType())?0.5:-0.5);
									neighbor->setSatisfaction(neighborUnscaledSatisfaction/neighborNSize, _threshold);

									// If neighbor is newly unsatisfied, add to _unsatisfied
									if(!(neighbor->isSatisfied()) && wasSatisfied) {
                                        unsatisfiedNeighbors.push_back(neighbor->getLocation());
									}
								}
								if((k == y) && (k == (x-1))) k++;
							}
						}

                        it_o = _openLocations.erase(it_o);
                        it_o--;
                        _openLocations.push_front(currentLocation);
                        // Removes relocated resident from unsatisfied list
                        it_u = _unsatisfied.erase(it_u);
                        it_u--;
						break;
					}
				}
			}
			//No location will satisfy the resident, so we keep them on the list
		} else {
            // Resident is already satisfied
			it_u = _unsatisfied.erase(it_u);
            it_u--;
		}
	}


    for(it_u = unsatisfiedNeighbors.begin(); it_u != unsatisfiedNeighbors.end(); it_u++) {
        Resident *r = _city->_map[*it_u];
        if(r && !(r->isSatisfied())) {
            _unsatisfied.push_back(*it_u);
        }
    }

    _iteration++;
	// Return 0 on success;
	return 0;
}

unsigned int Simulation::getIteration() {
    return _iteration;
}

unsigned int Simulation::numUnsatisfied() {
    return (unsigned int)_unsatisfied.size();
}

std::ostream& operator<< (std::ostream &out, const Simulation &sim) {
	out << "Iteration: " << sim._iteration << "\n" << *(sim._city) << "Threshold: " << sim._threshold << " Neighborhood radius: " << sim._city->getNeighborhoodRadius() << "\nUnsatisfied residents: " << sim._unsatisfied.size() << " Open Locations: " << sim._openLocations.size();
	return out;
}

int main(int argc, char* argv[]) {

	double popBreakdown[] = {0.5, 0.5};
	Simulation sim = Simulation(100, 100, 2, 8500, 2, popBreakdown);
	sim.placeResidents();
	sim.initializeSimulation(0.5);

	std::cout << sim << std::endl;

    while(sim.numUnsatisfied() && (sim.getIteration() < 1000)) {
        if(sim.iterate()) {
            break;
        }
        std::cout << sim << std::endl;
    }

    std::cout << "The simulation terminated in " << sim.getIteration() << " iteration(s)" << std::endl;

}
