//
//  EtherdreamDAC.cpp
//  etherdreamTest
//
//  Created by Seb Lee-Delisle on 08/03/2017.
//
//

#include "EtherdreamDAC.h"

void EtherdreamDAC :: setup(){
    pointsChanged = false;
    pointsBuffer = pointsA;
    pointsToSend = pointsB;
    startThread();
}

void EtherdreamDAC :: setPoints(const vector<ofxIlda::Point>& ildapoints, int _pps) {
    
    // TODO add size of points
    
    if (lock()){
        pps = _pps;
        pointsChanged = false;
        //fill_circle(ofGetElapsedTimef(), 0);
        for(int i =0; (i<ildapoints.size()) && (i<MAX_POINTS); i++){
            struct etherdream_point& pt = pointsBuffer[i];
            const ofxIlda::Point& ip = ildapoints[i];
            pt.x = ip.x;
            pt.y = ip.y;
            pt.r = ip.r;
            pt.g = ip.g;
            pt.b = ip.b;
            pt.i = 0;
            pt.u1 = 0;
            pt.u2 = 0;
            
        }
        numPointsInBuffer = MIN(ildapoints.size(), MAX_POINTS);
        
        pointsChanged = true;
        unlock();
    }
    

    
}

void EtherdreamDAC :: threadedFunction(){
    
    etherdream_lib_start();
    
    /* Sleep for a bit over a second, to ensure that we see broadcasts
     * from all available DACs. */
    usleep(1200000);
    
    int cc = etherdream_dac_count();
    while (!cc) {
        printf("No DACs found.\n");
        usleep(2400000);
        cc = etherdream_dac_count();
    }
    

    
    int i;
    for (i = 0; i < cc; i++) {
        printf("%d: Ether Dream %06lx\n", i,
               etherdream_get_id(etherdream_get(i)));
    }
    
    struct etherdream *d = etherdream_get(0);
    
    printf("Connecting...\n");
    while (etherdream_connect(d) < 0){
        usleep(2400000);
    }
        
    
    i = 0;
    while (1) {
       
        if(pointsChanged) {
            if(lock()) {
                
                struct etherdream_point* spare = pointsBuffer;
                pointsBuffer = pointsToSend;
                pointsToSend = spare;
                numPointsToSend = numPointsInBuffer;
                unlock();
                pointsChanged = false;
            }
            
        }
        int res = etherdream_write(d, pointsToSend, numPointsToSend, pps, 1);
        if (res != 0) {
            printf("write %d\n", res);
        }
        
        // TODO - make this a separate function
        while(etherdream_wait_for_ready(d)==-1) {
            printf("etherdream shut down!\n");
            etherdream_stop(d);
            etherdream_disconnect(d);
            usleep(1200000);
            d = etherdream_get(0);
            printf("retrying...");
            etherdream_connect(d);
            usleep(1200000);
        };
        
        i++;
    }
    
    printf("done\n");
    return 0;
    
}


uint16_t EtherdreamDAC ::  colorsin(float pos) {
    int res = (sin(pos) + 1) * 32768 *0.2;
    if (res < 0) return 0;
    if (res > 65535) return 65535;
    return res;
}
