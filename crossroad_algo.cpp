#include <iostream>
#include <vector>
#include <ctime>
#include <random>

std::mt19937 mtRand;

struct sCar;

const int stdFuelLvl = 11;
const int stdSpeed = 5;
const int stdCarWidth = 50;
const int stdCarHeigth = 100;
const int shiftFromCentral = 10;
const int distanceBeetweenCars = 125;
const int initialCarsCount = 10;
const int crossroadWidth = 80;	//radius

std::vector<sCar*> allCarsVec;	//можно было бы использовать std::unique_ptr<sCar>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

struct sRect
{
	int x;
	int y;
	int width;
	int height;

	bool intersects(const sRect  &other) const {

		if (this == &other) return false;

		return !((other.x + other.width <= x) ||
			(other.y + other.height <= y) ||
			(other.x >= x + width) ||
			(other.y >= y + height));
	}
};

enum class eDirection {
	UP,
	LEFT,
	RIGHT,
	DOWN,
	UNDEF
};

struct sCar {
private:

	sRect rect;
	eDirection dir = eDirection::UNDEF;
	int speed = stdSpeed;

public:

	sCar(int aX = 0, int aY = 0, int aWidth = stdCarWidth,
		int aHeigth = stdCarHeigth, eDirection aDir = eDirection::UNDEF,
		int aSpeed = stdSpeed) : rect{ aX, aY, aWidth, aHeigth }, dir(aDir),
		speed(aSpeed) {}

	virtual ~sCar() {}

	eDirection getDirection() const { return dir; }
	sRect getRect() const { return rect; }
	int getX() const { return rect.x; }
	int getY() const { return rect.y; }
	int getWidth() const { return rect.width; }
	int getHeigth() const { return rect.height; }

	virtual void move() {
		switch (dir) {
		case eDirection::UP:
			rect.y -= speed;
			return;
		case eDirection::DOWN:
			rect.y += speed;
			return;
		case eDirection::RIGHT:
			rect.x += speed;
			return;
		case eDirection::LEFT:
			rect.x -= speed;
			return;
		}
	}

	sRect getFuturePos() const {
		switch (dir) {
		case eDirection::UP:
			return sRect{ rect.x, rect.y - (speed + distanceBeetweenCars), rect.width, rect.height };
		case eDirection::DOWN:
			return sRect{ rect.x, rect.y + (speed + distanceBeetweenCars), rect.width, rect.height };
		case eDirection::RIGHT:
			return sRect{ rect.x + speed + distanceBeetweenCars, rect.y, rect.width, rect.height };
		case eDirection::LEFT:
			return sRect{ rect.x - (speed + distanceBeetweenCars), rect.y, rect.width, rect.height };
		default:
			std::cerr << "\nERROR OF GET_FUTURE_POSITION\n";
			return rect;
		}
	}

	bool needPassCarFromRight(const sCar* otherCar) const {
		sRect otherCarFuturePos = otherCar->getFuturePos();
		if (getFuturePos().intersects(otherCarFuturePos))	//right-hand priority
			switch (dir) {
			case eDirection::UP:
				otherCarFuturePos.height += crossroadWidth;
				if ((otherCar->dir == eDirection::LEFT) &&
					(rect.y > otherCarFuturePos.y + otherCarFuturePos.height))
					return true;
			case eDirection::DOWN:
				otherCarFuturePos.x += crossroadWidth;
				if ((otherCar->dir == eDirection::RIGHT) &&
					(rect.y + rect.height < otherCarFuturePos.y))
					return true;
			case eDirection::LEFT:
				otherCarFuturePos.width += crossroadWidth;
				if ((otherCar->dir == eDirection::DOWN) &&
					(rect.x > otherCarFuturePos.x + otherCarFuturePos.width))
					return true;
			case eDirection::RIGHT:
				otherCarFuturePos.x -= crossroadWidth + shiftFromCentral;
				if ((otherCar->dir == eDirection::UP) &&
					(rect.x + rect.width < otherCarFuturePos.x))
					return true;
			}
		return false;
	}

	bool needPassOtherCar(const sCar* otherCar) const {
		return (this != otherCar) &&
			getFuturePos().intersects(otherCar->rect) ||
			(needPassCarFromRight(otherCar));
	}

	bool inArea() const {
		switch (dir) {
		case eDirection::RIGHT:
			return rect.x < SCREEN_WIDTH;
		case eDirection::LEFT:
			return rect.x + rect.width > 0;
		case eDirection::UP:
			return rect.y + rect.height > 0;
		case eDirection::DOWN:
			return rect.y < SCREEN_HEIGHT;
		}
		return false;
	}

	void setLeftDirection() {
		std::swap(rect.height, rect.width);

		rect.x = SCREEN_WIDTH;
		rect.y = SCREEN_HEIGHT / 2 - (shiftFromCentral + rect.height);

		dir = eDirection::LEFT;
	}

	void setUpDirection() {
		rect.x = SCREEN_WIDTH / 2 + shiftFromCentral;
		rect.y = SCREEN_HEIGHT;

		dir = eDirection::UP;
	}

	void setDownDirection() {
		rect.x = SCREEN_WIDTH / 2 - (rect.width + shiftFromCentral);
		rect.y = 0 - rect.height;

		dir = eDirection::DOWN;
	}

	void setRightDirection() {
		std::swap(rect.height, rect.width);

		rect.x = 0 - rect.width;
		rect.y = SCREEN_HEIGHT / 2 + shiftFromCentral;

		dir = eDirection::RIGHT;
	}

	void replaceBehind(const sCar* otherCar) {
		switch (dir) {
		case eDirection::RIGHT:
			rect.x = otherCar->rect.x - (rect.width + distanceBeetweenCars);
			break;
		case eDirection::LEFT:
			rect.x = otherCar->rect.x + otherCar->rect.width + distanceBeetweenCars;
			break;
		case eDirection::UP:
			rect.y = otherCar->rect.y + otherCar->rect.height + distanceBeetweenCars;
			break;
		case eDirection::DOWN:
			rect.y = otherCar->rect.y - (distanceBeetweenCars + rect.height);
			break;
		default:
			std::cerr << "\nERROR OF REPLACE BEHIND\n";
		}
	}

	virtual int getFuel() const = 0;
	virtual void refill(int count) = 0;
};

struct sGasEngine : sCar {
private:

	int fuel = stdFuelLvl;

public:

	int getFuel() const { return fuel; }

	void refill(int count) { fuel += count; }

	void move() {
		if (fuel <= 0) refill(stdFuelLvl);
		fuel--; sCar::move();
	}
};

struct sElectroCar : sCar {
private:

	int charge = stdFuelLvl;

public:

	int getFuel() const { return charge; }

	void refill(int count) { charge += count; }

	void move() {
		if (charge <= 0) refill(stdFuelLvl);
		charge--; sCar::move();
	}
};

struct sHybrid : sCar {
private:

	int charge = (stdFuelLvl & 1) ? stdFuelLvl / 2 + 1 : stdFuelLvl / 2;
	int fuel = stdFuelLvl / 2;

public:

	int getFuel() const { return charge + fuel; }

	void refill(int count) {
		charge += (count & 1) ? count / 2 + 1 : count / 2;
		fuel += count / 2;
	}

	void move() {
		if (mtRand() % 2 == 0) {
			if (charge <= 0) refill(stdFuelLvl);
			charge--;
		}
		else {
			if (fuel <= 0) refill(stdFuelLvl);
			fuel--;
		}
		sCar::move();
	}
};



sCar* spawnRandomCar() {
	int carType = mtRand() % 3;
	switch (carType) {
	case 0:
		return new sGasEngine();
	case 1:
		return new sElectroCar();
	case 2:
		return new sHybrid();
	default:
		std::cerr << "ERROR OF SPAWN RANDOM CAR";
		return new sGasEngine();
	}
}

sCar* spawnCar() {
	int direction = mtRand() % 4;
	sCar* newCar = spawnRandomCar();
	switch (direction) {
	case 0:	newCar->setLeftDirection();
		break;
	case 1: newCar->setDownDirection();
		break;
	case 2:	newCar->setUpDirection();
		break;
	case 3:	newCar->setRightDirection();
		break;
	default: std::cerr << "ERROR OF SPAWN CAR";
	}

	for (const auto &existCar : allCarsVec) {
		if ((newCar->getDirection() == existCar->getDirection()) &&
			(existCar->getRect().intersects(newCar->getRect())))
		{
			newCar->replaceBehind(existCar);
		}
	}
	return newCar;
}

void respawnCar(sCar*& car) {   //FIXME не безопасное удаление, зделать через итератор

	delete car;
	car = nullptr;

	allCarsVec.erase(std::remove(begin(allCarsVec), end(allCarsVec), nullptr),
		end(allCarsVec));

	if (allCarsVec.size() < initialCarsCount) { 
		allCarsVec.push_back(spawnCar()); 
	}
	
}

auto getLeftCarIt() {
	return std::find_if(allCarsVec.begin(), allCarsVec.end(),
		[](const sCar* car) {return ((car->getDirection() == eDirection::RIGHT) &&
			(car->getX() < SCREEN_WIDTH / 2) &&
			(car->getX() >= SCREEN_WIDTH / 2 -
				(crossroadWidth + car->getHeigth() + distanceBeetweenCars))); });
}

bool leftCarExist() {
	return getLeftCarIt() != allCarsVec.end();
}
//
//bool allDirWait() {
//	auto leftCarIt = getLeftCarIt();
//
//	auto rightCarIt = std::find_if(allCarsVec.begin(), allCarsVec.end(),
//		[](const sCar* car) {return ((car->getDirection() == eDirection::LEFT) &&
//			(car->getX() > SCREEN_WIDTH / 2) &&
//			(car->getX() <= SCREEN_WIDTH / 2 +
//				crossroadWidth + distanceBeetweenCars));  });
//
//	auto upCarIt = std::find_if(allCarsVec.begin(), allCarsVec.end(),
//		[](const sCar* car) {return ((car->getDirection() == eDirection::DOWN) &&
//			(car->getY() < SCREEN_HEIGHT / 2) &&
//			(car->getY() >= SCREEN_HEIGHT / 2 -
//				(crossroadWidth + car->getHeigth() + distanceBeetweenCars))); });
//
//	auto downCarIt = std::find_if(allCarsVec.begin(), allCarsVec.end(),
//		[](const sCar* car) {return ((car->getDirection() == eDirection::UP) &&
//			(car->getY() > SCREEN_HEIGHT / 2) &&
//			(car->getY() <= SCREEN_HEIGHT / 2 +
//				crossroadWidth + distanceBeetweenCars)); });
//
//	return ((leftCarIt != allCarsVec.end()) &&
//		(rightCarIt != allCarsVec.end()) &&
//		(upCarIt != allCarsVec.end()) &&
//		(downCarIt != allCarsVec.end()));
//}


void leftCarMove() {
	auto leftCarIt = getLeftCarIt();
	if (leftCarExist()) {
		for (auto const &otherCar : allCarsVec) {
			if ((*leftCarIt)->getFuturePos().intersects(otherCar->getRect())) {
				return;
			}
		}
		(*leftCarIt)->move();
	}
}



void main_loop() {
	bool wasMove = false;
	for (auto &car : allCarsVec) {

		if (!car->inArea()) {
			respawnCar(car);
			continue;
		}
		
		for (size_t i = 0; i < allCarsVec.size(); i++) {
			if (car->needPassOtherCar(allCarsVec[i])) {
				break;
			}
			else if (i == allCarsVec.size() - 1) {
				car->move();
				wasMove = true;
			}
		}
		
	}
	if (!wasMove)
	{
		leftCarMove();
	}
}



int main(int argc, char** argv) {

	mtRand.seed(std::time(NULL));


	for (int i = 0; i < initialCarsCount; ++i) {
		allCarsVec.push_back(spawnCar());
	}
	
	while (true) { main_loop(); }

	return 0;
}
