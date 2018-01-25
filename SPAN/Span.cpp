#include "Span.h"

//Adapted version of the SPAN code from Alex which generates the binning and returns it as a vector of tupels
std::vector<std::pair<unsigned int, unsigned int> > SPAN::runSpan(Data& d, int s = 1, unsigned int maxCores = 1,bool verbose = false){
    int k = 0;
    bool useClasses = true;
    int parts = 1;
    
    //S::costfunptr ptr;
    //ptr = gaussian;
    Binning bins(d, verbose);
    // set up fractions
    std::vector<Fraction*> fractions;
    parts=round(log2(d.n)); //Added by fschmidt
    fractions.resize(parts); //Added by fschmidt
	if (verbose){
    std::cout<<"Spitting into "<<parts<<" subproblems"<<std::endl;
}
    if(parts > 1){
        omp_set_num_threads(fmin(parts,maxCores)); //Added by fschmidt
        int stdwidth = floor((double)d.n / (double)parts);
        #pragma omp parallel for
        for(int i = 0; i < parts; i++){
            int begin = i * stdwidth;
            int end = i == (parts-1) ? d.n : (i+1) * stdwidth;
            Fraction* f = new Fraction(begin, end, s, useClasses);
            bins.runSPAN(k, f);
  	  //fractions.push_back(f);
	  fractions[i]=f; //Added by fschmidt
    }
   }
	
	if (verbose){
	std::cout<<"Merging into a second layer"<<std::endl;
	}
	//Test if a second division layer to merge smaller cluster improves the required time.
	std::vector<Fraction*> second_layer;
	Fraction* current = NULL;
	for(int i =0; i < parts; i++){
		if(i % 5 == 0){ // oder welche Bedingung auch immer du magst
			if(i != 0){
				second_layer.push_back(current);
			}
			current = fractions[i];
		}else{
			current->mergeIn(fractions[i]);
		}
	}
	second_layer.push_back(current);  // last element
	if (verbose){
	std::cout<<"Second layer filled with "<<second_layer.size()<<std::endl;
}
	// algorithmus ausführen
	#pragma omp parallel for
	for(unsigned int i = 0; i < second_layer.size(); i++){
		bins.runSPAN(k, second_layer[i]);
	}

	if (verbose){
	std::cout<<"Merging results of second layer"<<std::endl;
}
        Fraction* fAll = second_layer.size() > 1 ? second_layer[0] : new Fraction(0, d.n, s, useClasses);
    for(unsigned int i = 1; i < second_layer.size(); i++){
        fAll->mergeIn(second_layer[i]);
    }
    //END second division layer

   //Generate vector of tupels holding final binning
   // run last run
    bins.runSPAN(k, fAll);
	
	std::vector<std::pair<unsigned int, unsigned int> > resultVector;
	for(unsigned int i = 0; i < fAll->seg->size(); i++){
 		resultVector.push_back(std::make_pair((*(fAll->seg))[i].start + 1,(*(fAll->seg))[i].end));
 	}

    if(parts > 1){
        for(int i = 0; i < parts; i++){
            delete fractions[i];
        }
    }

	return resultVector;
}

std::vector<std::pair<unsigned int, unsigned int> > SPAN::convertSegmentationToGenomicCoordinates(std::vector<std::pair<unsigned int, unsigned int> >& segmentation, std::tuple<std::string, unsigned int, unsigned int,std::string> genomicCoordinates){
	std::vector<std::pair<unsigned int, unsigned int> > transformedSegments;
	unsigned int correctionPosition = std::get<1>(genomicCoordinates)-1;
	for (auto& posPair : segmentation){
		transformedSegments.push_back(std::make_pair(posPair.first+correctionPosition,posPair.second+correctionPosition));
	}
	return transformedSegments;
}