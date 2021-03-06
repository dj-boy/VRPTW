#include <vector>
#include <algorithm>
#include "evolution.h"

using namespace std;

bool reduceRoute(solution &sol, const problem& input){
	if(sol.routes.size() <= 1) return false;
	unsigned int before = sol.routes.size();
	
	// find route with highest (distance / # of customers).
	list<route>::iterator minR;
	double dis = 0;
	for(list<route>::iterator it = sol.routes.begin(); it != sol.routes.end(); it++){
		double p = it->distance / it->visits.size();
		if(p > dis){
			minR = it;
			dis = p;
		}
	}

	// remove this shortest route
	route shortest = (*minR);
	sol.routes.erase(minR);

	for(list<int>::iterator cus = shortest.visits.begin(); cus != shortest.visits.end();){
		solution min = sol;

		for(int tryCount = 0; tryCount < input.getNumCusto(); tryCount++){
			solution temp = min;
			list<route>::iterator r = temp.routes.begin();
			advance(r, rand() % temp.routes.size() );
			list<int>::iterator ins = r->visits.begin();
			advance(ins, r->visits.size());
			r->visits.insert(ins, *cus);
			r->modified = true;
			temp.fitness(input);
			if(temp.totalDistance < min.totalDistance){
				min = temp;
			}
		}

		if(min.totalDistance < sol.totalDistance){
			cus = shortest.visits.erase(cus);
			shortest.modified = true;
			sol = min;
		}else{
			cus++;
		}
	}

	// append a new route with customers can't be inserted.
	if( !shortest.visits.empty() ){
		sol.routes.push_front(shortest);
	}

	return (sol.routes.size() < before);
}

solution crossover(const solution &pa, const solution &pb, const problem& input){
	solution offspring = pa;

	vector<route> bRoutes(pb.routes.begin(), pb.routes.end());
	// find best route with smallest ratio (distance / # of customers).
	unsigned int maxR = bRoutes.size();
	double dis = 1e100;
	for(unsigned int i = 0; i < bRoutes.size(); ++i){
		if(bRoutes[i].feasible){
			double p = bRoutes[i].distance / bRoutes[i].visits.size();
			if(p < dis){
				maxR = i;
				dis = p;
			}
		}
	}
	
	if(maxR == bRoutes.size() ) maxR = rand() % bRoutes.size();
	
	// remove best route's customer
	for(list<int>::iterator cus = bRoutes[maxR].visits.begin(); cus != bRoutes[maxR].visits.end(); cus++){
	for(list<route>::iterator it = offspring.routes.begin(); it != offspring.routes.end(); ++it){
		list<int>::iterator todel = find(it->visits.begin(), it->visits.end(), *cus);
		if( todel != it->visits.end() ){
			it->visits.erase(todel);
			it->modified = true;
			break;
		}
	}}

	// remove empty route
	for(list<route>::iterator it = offspring.routes.begin(); it != offspring.routes.end(); ){
		if(it->visits.size() == 0){
			it = offspring.routes.erase(it);
		}else{
			it++;
		}
	}

	offspring.routes.push_back(bRoutes[maxR]);

	while( reduceRoute(offspring, input) );

	offspring.fitness(input);
	return offspring;
}

void mutation(solution &sol, const problem& input){
	int tryCount = 0;
	while(tryCount < input.getNumCusto() ){
		solution test = sol;

		// randomly select two routes
		list<route>::iterator routeA = test.routes.begin();
		advance(routeA, rand() % test.routes.size() );
		list<route>::iterator routeB = test.routes.begin();
		advance(routeB, rand() % test.routes.size() );

		// # of feasible route BEFORE the reinsertion
		int beforeFeasibleCount = 0;
		if(routeA->feasible) beforeFeasibleCount++;
		if(routeB->feasible) beforeFeasibleCount++;

		// randomly select two positions
		list<int>::iterator cusA = routeA->visits.begin();
		advance(cusA, rand() % routeA->visits.size() );
		list<int>::iterator cusB = routeB->visits.begin();
		advance(cusB, rand() % routeB->visits.size() );

		routeB->visits.insert(cusB, *cusA);
		routeB->modified = true;
		routeA->visits.erase(cusA);
		routeA->modified = true;
		bool reduce = false;
		if( routeA->visits.empty() ){
			test.routes.erase(routeA);
			reduce = true;
		}

		test.fitness(input);
		
		// # of feasible route AFTER the reinsertion
		int afterFeasibleCount = 0;
		if(reduce || routeA->feasible) afterFeasibleCount++;
		if(routeB->feasible) afterFeasibleCount++;

		if(afterFeasibleCount >= beforeFeasibleCount) sol = test;

		tryCount++;
	}
}

// 2-tournament selection from population
const solution& tournament(const std::list<solution> &population, const problem &input){
	int selectA = rand() % population.size();
	int selectB = rand() % population.size();

	list<solution>::const_iterator itA = population.begin();
	advance(itA, selectA);
	list<solution>::const_iterator itB = population.begin();
	advance(itB, selectB);

	int cmp = solution::cmp( (*itA), (*itB), input);
	if(cmp == 0) cmp = (rand() % 2) * 2 - 1;   // -1 or 1

	if(cmp < 0) return (*itA);
	else return (*itB);
}

// Use Deb's "Fast Nondominated Sorting" (2002)
// Ref.: "A fast and elitist multiobjective genetic algorithm: NSGA-II"
void ranking(const std::list<solution> &population, std::vector< std::list<solution> > *output, bool feasible){
	if( population.empty() ) return;
	vector<solution> solutions(population.begin(), population.end() );

	vector< list<int> > intOutput;
	intOutput.resize(solutions.size() + 2);  // start from 1, end with null Qi 
	output->resize(1);

	// record each solutions' dominated solution
	vector< list<int> > dominated;
	dominated.resize(solutions.size());

	// record # of solutions dominate this solution
	vector<int> counter;
	counter.resize(solutions.size());

	// record the rank of solutions
	vector<int> rank;
	rank.resize(solutions.size());

	
	// for each solution
	for(unsigned int p = 0; p < solutions.size(); p++){
		for(unsigned int q = 0; q < solutions.size(); q++){
			if( feasible ){
				if( solution::fdominate(solutions[p], solutions[q]) ){
					dominated[p].push_back(q);  // Add q to the set of solutions dominated by p
				}else if( solution::fdominate(solutions[q], solutions[p]) ){
					counter[p]++;
				}
			}else{
				if( solution::idominate(solutions[p], solutions[q]) ){
					dominated[p].push_back(q);  // Add q to the set of solutions dominated by p
				}else if( solution::idominate(solutions[q], solutions[p]) ){
					counter[p]++;
				}
			}
		}
		
		if(counter[p] == 0){  // p belongs to the first front
			rank[p] = 1;
			intOutput[1].push_back(p);
			(*output)[0].push_back(solutions[p]);
		}
	}

	int curRank = 1;
	while( intOutput[curRank].size() > 0 ){
		list<int> Qi;  // Used to store the members of the next front
		list<solution> Qs;

		for(list<int>::iterator p = intOutput[curRank].begin(); p != intOutput[curRank].end(); p++){
		for(list<int>::iterator q = dominated[(*p)].begin(); q != dominated[(*p)].end(); q++){
			counter[(*q)]--;
			if(counter[(*q)] == 0){  // q belongs to the next front
				rank[(*q)] = curRank + 1;
				Qi.push_back(*q);
				Qs.push_back(solutions[(*q)]);
			}
		}}

		curRank++;
		intOutput[curRank] = Qi;
		if(Qi.size() > 0) output->push_back(Qs);
	}

	// remove duplicate solution in same rank
	for(unsigned int rank = 0; rank < output->size(); ++rank){
		(*output)[rank].sort();
		(*output)[rank].unique();
	}
}

void environmental(const vector< list<solution> > &frank, const vector< list<solution> > &irank, list<solution> *output, unsigned int maxSize){
	unsigned int curRank = 0;

	while(true){
		if(curRank < frank.size() && output->size() + frank[curRank].size() <= maxSize){
			output->insert(output->end(), frank[curRank].begin(), frank[curRank].end() );
		}else if(curRank < frank.size() ) break;

		if(curRank < irank.size() && output->size() + irank[curRank].size() <= maxSize){
			output->insert(output->end(), irank[curRank].begin(), irank[curRank].end() );
		}else if(curRank < irank.size() ) break;

		curRank++;
	}
	
	if(output->size() < maxSize && curRank < frank.size() ){
		vector<solution> nextRank(frank[curRank].begin(), frank[curRank].end() );

		while(output->size() < maxSize && !nextRank.empty()){
			unsigned int select = rand() % nextRank.size();
			output->push_back(nextRank[select]);
			nextRank.erase(nextRank.begin() + select);
		}
	}
	if(output->size() < maxSize && curRank < irank.size() ){
		vector<solution> nextRank(irank[curRank].begin(), irank[curRank].end() );

		while(output->size() < maxSize && !nextRank.empty()){
			unsigned int select = rand() % nextRank.size();
			output->push_back(nextRank[select]);
			nextRank.erase(nextRank.begin() + select);
		}
	}
}