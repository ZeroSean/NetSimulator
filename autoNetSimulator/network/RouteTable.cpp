#include "RouteTable.h"

#include <QDebug>

RouteTable::RouteTable(int maxSize) :
    _uploadGateway()
{
    _maxSize = maxSize;
    _size = 0;
    _next = -1;

    _table = new RouteElement[maxSize];

    _uploadGateway.depth = 0xff;
    _uploadGateway.dest = 0xffff;
    _uploadGateway.out = 0xffff;
    _uploadGateway.isGateway = 0;
}

RouteTable::~RouteTable() {
    if(_table != NULL) {
        delete[] _table;
        _table = NULL;
    }
}

int RouteTable::RouteSize(void) {
    return _size;
}

bool RouteTable::RouteEmpty(void) {
    return (_size == 0);
}

void RouteTable::update(uint16_t dest, uint16_t out, uint8_t depth, uint8_t isGateway) {
    RouteElement *element = this->find(dest);

    if(element != NULL) {
        if(depth <= element->depth) {
            element->depth = depth;
            element->out = out;
            element->isGateway = isGateway;
        }
    } else if(_size < _maxSize){
        int i = _size - 1;

        for(; i >= 0; --i) {
            if(_table[i].dest > dest) {
                _table[i + 1] = _table[i];
            } else {
                break;
            }
        }
        _table[i + 1].dest = dest;
        _table[i + 1].depth = depth;
        _table[i + 1].out = out;
        _table[i + 1].isGateway = isGateway;

        element = &_table[i + 1];

        _size += 1;
    } else {
        qDebug() << "route table no has empty space, dest route:" << dest;
    }

    //维护最短路径的网关
    if(isGateway && _uploadGateway.depth >= depth) {
        memcpy(&_uploadGateway, element, sizeof(RouteElement));
    } else if((isGateway == 0) && (_uploadGateway.dest == dest)) {
        int min = -1;

        for(int i = 0; i < _size; ++i) {
            if(_table[i].isGateway) {
                if(min == -1 || _table[min].depth > _table[i].depth) {
                    min = i;
                }
            }
        }

        if(min == -1) {
            _uploadGateway.isGateway = 0;
            _uploadGateway.depth = 0xff;
        } else {
            memcpy(&_uploadGateway, &_table[min], sizeof(RouteElement));
        }
    }
}

RouteElement * RouteTable::find(uint16_t dest) {
    int left = 0, right = _size - 1;

    while(left <= right) {
        int mid = left + (right - left) / 2;

        if(_table[mid].dest == dest) {
            return &_table[mid];
        } else if(_table[mid].dest > dest) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }

    return NULL;
}

const RouteElement * RouteTable::cfind(uint16_t dest) {
    return this->find(dest);
}

const RouteElement * RouteTable::at(uint16_t index) {
    if(index < _size) {
        return &_table[index];
    }
    return NULL;
}

const RouteElement * RouteTable::findShortestGateway() {
    return &_uploadGateway;
}

uint8_t RouteTable::getGatewayDepth() {
    return _uploadGateway.depth;
}

uint16_t RouteTable::getGatewayAddr() {
    return _uploadGateway.dest;
}

uint16_t RouteTable::getGatewayOut() {
    return _uploadGateway.out;
}

const RouteElement * RouteTable::getNextElement() {
    if(_size > 0) {
        _next = (_next + 1) % _size;
        return &_table[_next];
    } else {
        return &_uploadGateway;
    }
}
