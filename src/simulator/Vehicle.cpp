///   EN: Project for OOP subject at Warsaw University of Technology
///       City traffic simulation
///
///   PL: Projekt PROI (Programowanie obiektowe) PW WEiTI 18L
///       Symulacja ruchu miejskiego
///
///   Copyright (C) Robert Dudzinski 2018
///
///   File: Vehicle.cpp


#include "Vehicle.h"
#include "Road.h"

class Driveable;

const Vec3 Vehicle::blinkerColor = Vec3(1, 0.647, 0);

Vehicle::Vehicle(Driveable *spawnRoad)
{
    initRandValues();

    velocity = 0;
    specs.vehicleLength = 0.2;
    xPos = 0;

    isBraking = false;

    dstToCross = 1000;
    direction = true;
    desiredTurn = 0;

    blinker.init();

    initPointers(spawnRoad);

    rot = Vec3(0, curRoad->getDirection().angleXZ(), 0);

    color = Colors::getRandomColor();
    color *= 0.70f;
}

void Vehicle::initRandValues()
{
    specs.maxV = randFloat(1, 1.5);
    specs.minV = randFloat(0.02, 0.08);
    specs.cornerVelocity = 1;
    specs.stopTime = randFloat(0.5, 0.8);
    specs.acceleration = randFloat(0.1,0.2);
    specs.remainDst = randFloat(0.06, 0.08);
}

void Vehicle::Blinker::init()
{
    which = 0;
    time = 0;
    duration  = randFloat(0.45,0.55);
    isLighting = true;
}

void Vehicle::initPointers(Driveable *spawnRoad)
{
    curRoad = spawnRoad;
    curCross = nullptr;
    nextRoad = nullptr;

    backVeh = nullptr;

    allowedToCross = false;
    crossState.isChanging = false;
    crossState.didReachCross = false;
    crossState.isLeavingRoad = false;

    frontVeh = NULL;
    isFirstVeh = true;

    if (curRoad->vehiclesBeg.size() > 0 /*&& !curRoad->vehiclesBeg.back()->isChanging*/)
    {
        isFirstVeh = false;
        frontVeh = curRoad->vehiclesBeg.back();
        frontVeh->backVeh = this;
    }
}

void Vehicle::update(const float delta)
{
    if (!crossState.isChanging && !crossState.didReachCross)
    {
        float prevVelocity = velocity;

        xPos += velocity * delta;

        setVelocity();
        checkVelocity(delta, prevVelocity);
        setNewPos();

        if (curRoad->getLength() - xPos < 2.4 && curCross == nullptr)
        {
            registerToCross();
        }
    }

    if (!crossState.isLeavingRoad && curCross != nullptr && nextRoad != nullptr && allowedToCross/* && nextRoad->freeSpace(direction) > vehicleLength + remainDst*/)
    {
        tryBeAllowedToEnterCross();
    }

    if (crossState.isChanging)
    {
        xPos += specs.cornerVelocity * delta;

        float s = xPos / curRoad->getLength();

        if (direction)
        {
            pos = Vec3::lerp(curRoad->getBegJointWidth(direction), curRoad->getEndJointWidth(direction), s);
        }
        else
        {
            pos = Vec3::lerp(curRoad->getEndJointWidth(direction), curRoad->getBegJointWidth(direction), s);
        }

        if(s > 1)
        {
            leaveRoad();
        }
    }

    if (crossState.didReachCross)
    {
        xPos += specs.cornerVelocity * delta;

        setCornerPosition();

        if (crossState.crossProgress>=1)
        {
            enterNewRoad();
        }
    }

    blinker.updateBlinkers(delta);
}

void Vehicle::setVelocity()
{
    float posDiff = getDst() - specs.vehicleLength / 2.0 - specs.remainDst;
    float newDst = velocity * specs.stopTime - specs.acceleration * specs.stopTime * specs.stopTime / 2.0;

    if (newDst > posDiff)
    {
        velocity = posDiff - specs.acceleration * specs.stopTime * specs.stopTime / 2.0;
        velocity /= specs.stopTime;
    }
    else if (posDiff > newDst)
    {
        velocity = posDiff - specs.acceleration * specs.stopTime * specs.stopTime / 2.0;
        velocity /= specs.stopTime;
    }
}

void Vehicle::checkVelocity(const float delta, float prevVelocity)
{
    float braking = (velocity - prevVelocity) / delta;

    if (braking < -0.3)
    {
        isBraking = true;
    }
    else
    {
        isBraking = false;
    }

    if (velocity < specs.minV)
    {
        velocity = 0;
        isBraking = true;
    }

    if (velocity > specs.maxV)
    {
        velocity = specs.maxV;
    }
}

float Vehicle::getXPos() const
{
    return xPos;
}

float Vehicle::getDstToCross() const
{
    return dstToCross;
}

void Vehicle::setNewPos()
{

    if(xPos > curRoad->getLength())
    {
        xPos = curRoad->getLength();
    }

    float s = xPos / curRoad->getLength();

    if (s>1)
    {
        s=1;
    }

    if (direction)
    {
        pos = Vec3::lerp(curRoad->getBegJointWidth(direction), curRoad->getEndJointWidth(direction), s);
    }
    else
    {
        pos = Vec3::lerp(curRoad->getEndJointWidth(direction), curRoad->getBegJointWidth(direction), s);
    }

    dstToCross = curRoad->getLength() - xPos;
}

void Vehicle::registerToCross()
{
    allowedToCross = false;

    if(direction)
    {
        curCross = curRoad->crossEnd;
    }
    else
    {
        curCross = curRoad->crossBeg;
    }

    blinker.which = 0;

    if (curCross != nullptr)
    {
        for (unsigned int i=0;i<curCross->streets.size();i++)
        {
            if (curCross->streets[i].street == curRoad)
            {
                desiredTurn = randInt(0, curCross->streets.size()-1);
                if (desiredTurn == (int)i) desiredTurn = (desiredTurn+1) % curCross->streets.size();

                if (curCross->streets.size() == 2) desiredTurn = (i+1) % 2;

                nextRoad = curCross->streets[desiredTurn].street;

                crossState.begRot = curRoad->direction.angleXZ();
                crossState.endRot = nextRoad->direction.angleXZ();

                if (!direction) crossState.begRot += 180;
                if (!curCross->streets[desiredTurn].direction) crossState.endRot += 180;

                blinker.which = rotateDirection(crossState.begRot, crossState.endRot);
                if (curCross->streets.size() == 2) blinker.which = 0;

                blinker.time = 0;
                blinker.isLighting = true;

                curCross->streets[i].vehicles.push_back(this);

                break;
            }
        }
    }
}

void Vehicle::tryBeAllowedToEnterCross()
{
    for (auto &street : curCross->streets)
    {
        if (street.street == nextRoad)
        {
            if (nextRoad->freeSpace(street.direction) > specs.vehicleLength + specs.remainDst)
            {
                crossState.isLeavingRoad = true;

                if (street.direction)
                {
                    nextRoad->reservedSpaceBeg += specs.vehicleLength + specs.remainDst;
                }
                else
                {
                    nextRoad->reservedSpaceEnd += specs.vehicleLength + specs.remainDst;
                }

                crossState.isChanging = true;
                crossState.didReachCross = false;
            }

            break;
        }
    }
}

void Vehicle::leaveRoad()
{
    xPos = 0;
    crossState.didReachCross = true;
    crossState.isChanging = false;

    if (curCross->streets[desiredTurn].direction)
    {
        nextRoadJoint = nextRoad->getBegJointWidth(curCross->streets[desiredTurn].direction);
    }
    else
    {
        nextRoadJoint = nextRoad->getEndJointWidth(curCross->streets[desiredTurn].direction);
    }

    if(backVeh != nullptr)
    {
        backVeh->isFirstVeh = true;
        backVeh->frontVeh = nullptr;
    }

    crossState.begRot = curRoad->direction.angleXZ();
    crossState.endRot = nextRoad->direction.angleXZ();

    if (!direction) crossState.begRot += 180;
    if (!curCross->streets[desiredTurn].direction) crossState.endRot += 180;
}

void Vehicle::setCornerPosition()
{
    float tempLength;
    float s;

    if (direction)
    {
        tempLength = Vec3::length(curRoad->getEndJointWidth(direction) - nextRoadJoint);
        s = xPos / tempLength;

        if(s>1)s=1;

        pos  = Vec3::lerp(curRoad->getEndJointWidth(direction), nextRoadJoint, s);
    }
    else
    {
        tempLength = Vec3::length(curRoad->getBegJointWidth(direction) - nextRoadJoint);
        s = xPos / tempLength;

        if(s>1)s=1;

        pos  = Vec3::lerp(curRoad->getBegJointWidth(direction), nextRoadJoint, s);
    }

    rot = Vec3(0, lerpAngle(crossState.begRot, crossState.endRot, s), 0);
    crossState.crossProgress = s;
}

void Vehicle::enterNewRoad()
{
    blinker.which = 0;

    if(backVeh != nullptr)
    {
        backVeh->isFirstVeh = true;
        backVeh->frontVeh = nullptr;
    }

    if (direction)
    {
        curRoad->vehiclesBeg.pop();
    }
    else
    {
        curRoad->vehiclesEnd.pop();
    }

    xPos = 0;
    isBraking = false;

    allowedToCross = false;

    crossState.isChanging = false;
    crossState.didReachCross = false;
    crossState.isLeavingRoad = false;

    velocity = specs.cornerVelocity;

    curRoad = curCross->streets[desiredTurn].street;

    direction = curCross->streets[desiredTurn].direction;

    //dstToCross = curCross->getLength();

    if (direction)
    {
        nextRoad->reservedSpaceBeg -= specs.vehicleLength + specs.remainDst;
    }
    else
    {
        nextRoad->reservedSpaceEnd -= specs.vehicleLength + specs.remainDst;
    }

    curCross->allowedVeh--;
    desiredTurn = 0;

    backVeh = nullptr;

    frontVeh = nullptr;
    isFirstVeh = true;

    if (direction)
    {
        if (curRoad->vehiclesBeg.size() > 0)
        {
            isFirstVeh = false;
            frontVeh = curRoad->vehiclesBeg.back();
            frontVeh->backVeh = this;
        }
    }
    else
    {
        if (curRoad->vehiclesEnd.size() > 0)
        {
            isFirstVeh = false;
            frontVeh = curRoad->vehiclesEnd.back();
            frontVeh->backVeh = this;
        }
    }

    if (direction)
    {
        curRoad->vehiclesBeg.push(this);
    }
    else
    {
        curRoad->vehiclesEnd.push(this);
    }

    nextRoad = nullptr;
    curCross = nullptr;
}

void Vehicle::Blinker::updateBlinkers(const float delta)
{
    time += delta;

    if (time > duration)
    {
        time = 0;
        isLighting = !isLighting;
    }
}

float Vehicle::getDst() const
{
    if (frontVeh != nullptr)
        return frontVeh->xPos - xPos - frontVeh->specs.vehicleLength/2.0;

    return curRoad->getLength() - xPos;
}

bool Vehicle::isEnoughSpace() const
{
    if (nextRoad == nullptr || curCross == nullptr || desiredTurn >= (int)curCross->streets.size()) return false;

    return nextRoad->freeSpace(curCross->streets[desiredTurn].direction) > specs.vehicleLength + specs.remainDst;
}

Car::Car(Driveable *spawnRoad) : Vehicle(spawnRoad)
{
    velocity = randFloat(2,5);
}

void Car::update(float delta)
{
    Vehicle::update(delta);
}

void Car::draw()
{
    translate(0, -0.02, 0);

    if (blinker.which < 0 && blinker.isLighting)
    {
        pushMatrix();
        translate(0,0.05,-0.038);
        setColor(blinkerColor);
        drawCube(0.22,0.02,0.01);
        popMatrix();
    }
    if (blinker.which > 0 && blinker.isLighting)
    {
        pushMatrix();
        translate(0,0.05,0.038);
        setColor(blinkerColor);
        drawCube(0.22,0.02,0.01);
        popMatrix();
    }

    if (isBraking)
    {
        setColor(1,0,0);

        pushMatrix();
        translate(-0.05,0.08,0);
        drawCube(0.07,0.003,0.04);
        popMatrix();

        pushMatrix();
        translate(-0.05,0.05,0.033);
        drawCube(0.12,0.01,0.01);
        popMatrix();

        pushMatrix();
        translate(-0.05,0.05,-0.033);
        drawCube(0.12,0.01,0.01);
        popMatrix();
    }
    setColor(color);

    pushMatrix();
    translate(0,0.05,0);
    drawCube(0.2,0.05,0.1);
    drawRoof();

    popMatrix();
}

void Car::drawRoof()
{
    Vec3 a1(0,0,-0.05);
    Vec3 a2(0.025,0.05,-0.0375);
    Vec3 a3(0.075,0.05,-0.0375);
    Vec3 a4(0.1125,0,-0.05);
    Vec3 a5(0,0,0.05);
    Vec3 a6(0.025,0.05,0.0375);
    Vec3 a7(0.075,0.05,0.0375);
    Vec3 a8(0.1125,0,0.05);

    pushMatrix();
    translate(-0.075, 0.025, 0);
    beginDraw(QUADS);

    drawQuad(a2,a6,a7,a3);

    setColor(0,1,1);

    drawQuad(a1,a2,a3,a4);
    drawQuad(a1,a5,a6,a2);
    drawQuad(a5,a8,a7,a6);
    drawQuad(a8,a4,a3,a7);

    endDraw();
    popMatrix();
}

Bus::Bus(Driveable *spawnRoad) : Vehicle(spawnRoad)
{
    specs.maxV = randFloat(0.8, 1.1);
    velocity = randFloat(2,5);

    specs.vehicleLength = 0.66;
    specs.remainDst = randFloat(0.14, 0.15);

    color = Vec3(0.7, 0.7, 0);
}

void Bus::update(const float delta)
{
    Vehicle::update(delta);

    if (crossState.didReachCross)
    {
        float s = crossState.crossProgress * 2.0 - 1.0;
        s = 1 - abs(s);
        float a = Vec3::angleDiff(crossState.begRot, crossState.endRot);
        a /= 4.0;

        busAngle = lerpAngle(0, a, s);
    }
    else
    {
        busAngle = 0;
    }
}

void Bus::draw()
{
    pushMatrix();

    translate(0,0.07,0);

    setColor(color);
    pushMatrix();
    rotateY(-busAngle / 1.3);
    translate(-0.2,0,0);
    drawCube(0.3,0.13,0.135);
    setColor(0,0.8,0.8);
    translate(-0.02,0.02,0);
    drawCube(0.25,0.07,0.14);
    popMatrix();

    setColor(color);
    pushMatrix();

    rotateY(busAngle / 4);
    translate(0.2,0,0);
    drawCube(0.3, 0.13, 0.135);
    setColor(0,0.8,0.8);
    translate(0.02,0.02,0);
    drawCube(0.27,0.07,0.14);
    popMatrix();

    setColor(0.5,0.5,0);
    drawCube(0.2,0.12,0.12);

    pushMatrix();
    if (blinker.which < 0 && blinker.isLighting)
    {
        setColor(blinkerColor);
        translate(0, -0.031,-0.046);
        drawCube(0.73,0.01,0.01);
    }
    if (blinker.which > 0 && blinker.isLighting)
    {
        setColor(blinkerColor);
        translate(0, -0.031,0.046);
        drawCube(0.73,0.01,0.01);
    }
    popMatrix();

    if (isBraking)
    {
        pushMatrix();
        setColor(1,0,0);
        rotateY(-busAngle / 1.3);

        pushMatrix();
        translate(-0.3,0.05,0);
        drawCube(0.12,0.003,0.06);
        popMatrix();

        pushMatrix();
        translate(-0.3,-0.02,0.04);
        drawCube(0.12,0.01,0.01);
        popMatrix();

        pushMatrix();
        translate(-0.3,-0.02,-0.04);
        drawCube(0.12,0.01,0.01);
        popMatrix();
        popMatrix();
    }

    popMatrix();
}
