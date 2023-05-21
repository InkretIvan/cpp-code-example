//Ivan Inkret, 0036535014
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <utility>
#include <queue>
#include <sstream>
#include <set>
#include <map>
#include <stack>
#include <math.h>

using namespace std;

struct node{
	string znacajka;
	vector<node*> subtrees;
	node* parent=nullptr;
	bool isLeaf=false;
	string klasaIfLeaf;
	string ZnacajkaDoOvdje;
};

vector<string> parse(string str, string delimiter); //parse po delimiteru
double ent(vector<int> x); //vracamo entropiju za neki skup
void readFiles(string p1, string p2);
bool vContains(vector<string> v, string find);
node* id3(vector<vector<string> > D, vector<vector<string> > Dp, vector<string> X, string Y, vector<bool> wl, int depth); //algoritam id3 za izgradnju stabla
string argmaxK(vector<string> vals, vector<vector<string> > examples); //vraca ciljnu klasu koja se najcesce pojavljuje u primjerima
int argmaxE(vector<vector<string> > examples, vector<string> tokens, vector<bool> wl); //racuna najdiskriminativniju znacajku u nekom skupu primjera i vraæa njezin indeks
bool equalVecs(vector<vector<string> > v, string token); //vraca je li ulazni skup primjera isti kao i kad bi ga filtrirali po ciljnoj klasi token, tj je li za svaki primjer
int getClassByName(string name);                         //ciljna klasa token

vector<bool> znacajkeWhitelist;
void preorder(node* r);
stack<node*> stek;

int maxDepth=INT_MAX; 

vector<string> znacajkeIme;
string klasaIme;

vector<vector<string> > znacajkeVrijednosti;
vector<string> klasaVrijednosti;
 
vector< vector<string> > primjerUcenje;
vector< vector<string> > primjerTest;

vector<node*> branches;
vector<node*> nodes;


class id3Class{
	private:
		node *root;
		
	public:
		id3Class(){
		}
		
		void train(vector< vector<string> > train_dataset){
			root=id3(train_dataset,train_dataset,znacajkeIme,klasaIme,znacajkeWhitelist,0);
			cout << "[BRANCHES]:" << endl;
			preorder(root);
		}
		
		void predict(vector<vector<string> > test_dataset){
			
			int brojKlasa=klasaVrijednosti.size();
			int matrica[brojKlasa][brojKlasa]; 
			
			for(int ki=0;ki<brojKlasa;ki++){
				for(int kj=0;kj<brojKlasa;kj++){
					matrica[ki][kj]=0;
				}
			}
			
			int siz=test_dataset.size();
			int corr=0;  //broj toènih predikcija
			cout << "[PREDICTIONS]: ";
			for(int i=0;i<test_dataset.size();i++){
				vector<string> primjer=test_dataset[i];
				
				node* curr=root;
				
				while(curr!=nullptr){
					if(curr->isLeaf){  //u listu smo, možemo donijeti odluku
						cout << curr->klasaIfLeaf << " ";
						
						string predvidenaKlasa=curr->klasaIfLeaf;
						string stvarnaKlasa=primjer[primjer.size()-1];
						
						int ind1=getClassByName(stvarnaKlasa);
						int ind2=getClassByName(predvidenaKlasa);
						
						matrica[ind1][ind2]++;
						
						if(predvidenaKlasa==stvarnaKlasa) corr++;
						
						break;
					}
					
					string z=curr->znacajka;
					int index;
					
					for(int j=0;j<znacajkeIme.size();j++){
						if(znacajkeIme[j]==z){
							index=j;
							break;
						}
					}
					
					string testVal=primjer[index];
					
					int j=0;
					for(j;j<znacajkeVrijednosti[index].size();j++){
						string val=znacajkeVrijednosti[index][j];
						if(val==testVal){ //kreæemo se u neko podstablo
							curr=curr->subtrees[j];
							j=0;
							break;
						}
					}
					
					if(j==znacajkeVrijednosti[index].size()){  //vrijednost znacajke koju nikad nismo vidjeli, demokratska odluka
						cout << curr->klasaIfLeaf << " ";
						
						string predvidenaKlasa=curr->klasaIfLeaf;
						string stvarnaKlasa=primjer[primjer.size()-1];
						
						int ind1=getClassByName(stvarnaKlasa);
						int ind2=getClassByName(predvidenaKlasa);
						
						matrica[ind1][ind2]++;
						
						if(predvidenaKlasa==stvarnaKlasa) corr++;
						
						break;
					}
					
				}
				
			}
			cout << endl;
			
			double acc=(double)corr/(double)siz;
			
			printf("[ACCURACY]: %.5lf\n",acc);
			
			cout << "[CONFUSION_MATRIX]: " << endl;
			for(int ki=0;ki<brojKlasa;ki++){
				for(int kj=0;kj<brojKlasa;kj++){
					cout << matrica[ki][kj] << " ";
				}
				cout << endl;
			}
		}		
};

int main(int argc, char *argv[]) {
	
	string path1,path2;

	if(argc>2) path1=argv[1];
	if(argc>2) path2=argv[2];
	if(argc>3) maxDepth=stoi(argv[3]);
	
	readFiles(path1, path2);
	
	id3Class model;
	model.train(primjerUcenje);
	model.predict(primjerTest);
	
	for(int i=0;i<nodes.size();i++){
		delete nodes[i];
	} 
	
	return 0;
}

vector<string> parse(string str, string delimiter){
	vector<string> ret;
	
	auto start = 0U;
    auto end = str.find(delimiter);
    while (end != std::string::npos)
    {
        ret.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    ret.push_back(str.substr(start, end));
    return ret;
}

double ent(vector<int> x){
	double r=0,t=0;
	
	for(int i=0;i<x.size();i++){
		t=t+(double)x[i];
	}
	if(t==0) return 0;
	
	for(int i=0;i<x.size();i++){
		double p=(double)x[i]/t;
		if(p==0) continue;
		r=r+p*log2(p);
	}
	
	if(!r) return r;
	return -r;
}

void readFiles(string p1, string p2){
	//p1
	fstream newFile;
	string line;
	vector<string> lines;
	newFile.open(p1, ios::in);
		
	while (getline(newFile, line)) {
		lines.push_back(line);
    }
		
	newFile.close();
	
	vector<string> emptyy;
	for(int i=0;i<lines.size()-1;i++) primjerUcenje.push_back(emptyy);
	
	for(int i=0;i<lines.size();i++){
		vector<string> v=parse(lines[i],",");
		if(i==0){
			for(int j=0;j<v.size();j++){
				if(j==v.size()-1){
					klasaIme=v[j];
				}
				else{
					znacajkeIme.push_back(v[j]);
					znacajkeWhitelist.push_back(true);
					vector<string> emptyy;
					znacajkeVrijednosti.push_back(emptyy);
				}
			}
			
			continue;
		}
		
		for(int j=0;j<v.size();j++){
			if(j==v.size()-1){
				if(!vContains(klasaVrijednosti,v[j])) klasaVrijednosti.push_back(v[j]);
				primjerUcenje[i-1].push_back(v[j]);
				}
			else{
				if(!vContains(znacajkeVrijednosti[j],v[j])) znacajkeVrijednosti[j].push_back(v[j]);
				primjerUcenje[i-1].push_back(v[j]);
			}
			
		}
		
	}


	
	sort(klasaVrijednosti.begin(),klasaVrijednosti.end());
	
	//p2
	
	fstream newFile2;
	lines.clear();
	newFile2.open(p2, ios::in);
		
	while (getline(newFile2, line)) {
		lines.push_back(line);
    }

	newFile2.close();
	
	for(int i=0;i<lines.size()-1;i++) primjerTest.push_back(emptyy);

	
	for(int i=0;i<lines.size();i++){
		vector<string> v=parse(lines[i],",");
		if(i==0){	
			continue;
		}
		
		for(int j=0;j<v.size();j++){
			if(j==v.size()-1){
				primjerTest[i-1].push_back(v[j]);
				}
			else{
				primjerTest[i-1].push_back(v[j]);
			}
			
		}
		
	}
	
}

bool vContains(vector<string> v, string find){
	for(int i=0;i<v.size();i++){
		if(v[i]==find) return true;
	}
	return false;
}

node* id3(vector<vector<string> > D, vector<vector<string> > Dp, vector<string> X, string Y, vector<bool> wl, int depth){ //po pseudokodu iz prezentacija

	if(D.empty()){
		node *newLeaf= new node;
		nodes.push_back(newLeaf);
		newLeaf->isLeaf=true;
		
		string predict=argmaxK(klasaVrijednosti, Dp);
		newLeaf->klasaIfLeaf=predict;

		return newLeaf;
	}
	
	string predict=argmaxK(klasaVrijednosti, D);
	
	
	if(X.empty() || equalVecs(D,predict) || depth==maxDepth){ //3. argument nadodan na pseudokod za ogranicavanje dubine
		node* newLeaf = new node;
		nodes.push_back(newLeaf);
		newLeaf->isLeaf=true;
		newLeaf->klasaIfLeaf=predict;

		return newLeaf;
	} 
	
	node *ret = new node;
	nodes.push_back(ret);
			
	int x=argmaxE(D,X,wl);
	wl[x]=false;
	
	ret->znacajka=znacajkeIme[x];
	ret->klasaIfLeaf=argmaxK(klasaVrijednosti,D); //ovaj cvor nije list ali u klasaIfLeaf zapisujemo demokratsku odluku ako tu moramo stati
		
	vector<node*> subtrees;

	int brojVrijednosti=znacajkeVrijednosti[x].size();
	
	for(int i=0;i<brojVrijednosti;i++){
		string val=znacajkeVrijednosti[x][i];
		
		vector<string> xNovi;
		for(int j=0;j<X.size();j++) if(X[j]!=znacajkeIme[x]) xNovi.push_back(X[j]);
		
		vector<vector<string> > dNovi;
		for(int j=0;j<D.size();j++){
			if(D[j][x]==znacajkeVrijednosti[x][i]){ // ??
				dNovi.push_back(D[j]);
			}
		}
		
		node *t=id3(dNovi,D,xNovi,Y,wl,depth+1); //REKURZIVNI POZIV
		
	
		t->parent=ret;
		t->ZnacajkaDoOvdje=val;
		ret->subtrees.push_back(t);
		//ret->vrijednostiZnacajke.push_back(val);
	}
	
	return ret;

}

string argmaxK(vector<string> vals, vector<vector<string> > examples){
	
	int index=0;
	int maxSoFar=-1;
	int occ[vals.size()];
	for(int i=0;i<vals.size();i++) occ[i]=0;
	
	for(int i=0;i<examples.size();i++){
		int s=examples[i].size()-1;
		string cilj=examples[i][s];
		for(int j=0;j<vals.size();j++){
			if(cilj==vals[j]){
				occ[j]=occ[j]+1;
				break;
			}
		}	
	}
	
	maxSoFar=occ[0];
	
	for(int i=1;i<vals.size();i++){
		if(occ[i]>maxSoFar){
			maxSoFar=occ[i];
			index=i;
		}
	}
	
	return vals[index];
}

int argmaxE(vector<vector<string> > examples, vector<string> tokens, vector<bool> wl){
	int siz=tokens.size();
	//cout << "siz: " << siz << endl;
	int ex=examples.size();
	double IG[znacajkeIme.size()];
	double brojPrimjera=(double)siz;
	
	
	//entropija pocetnog skupa
	double Ep;
	vector<int> cilj;
	for(int i=0;i<klasaVrijednosti.size();i++) cilj.push_back(0);
	for(int i=0;i<examples.size();i++){
		string k=examples[i][examples[i].size()-1];
		for(int j=0;j<klasaVrijednosti.size();j++){
			if(k==klasaVrijednosti[j]){
				cilj[j]=cilj[j]+1;
				break;
			}
		}
	}
	Ep=ent(cilj);
	for(int i=0;i<znacajkeIme.size();i++) IG[i]=Ep;
	
	//za svaku znacajku
	
	
	
	for(int z=0;z<znacajkeIme.size();z++){ //za svaku znacajku
	
		if(!wl[z]){
			IG[z]=-1;
			continue;
		}
		
		for(int v=0;v<znacajkeVrijednosti[z].size();v++){ //za svaku njezinu vrijednost
			double reduction=0;
			string val=znacajkeVrijednosti[z][v];
			int brojPojavljivanja=0;
			
			vector<int> vec;
			for(int u=0;u<klasaVrijednosti.size();u++) vec.push_back(0);
			
			for(int i=0;i<ex;i++){ //gledamo cijeli ulazni skup
				double exd=(double)ex;
				
				if(examples[i][z]==val){
					brojPojavljivanja++;
					for(int j=0;j<klasaVrijednosti.size();j++){ //gledamo kojoj klasi pripada primjer
						if(klasaVrijednosti[j]==examples[i][examples[i].size()-1]){
							vec[j]=vec[j]+1;
							break;
						}	
					}
				}	
			}
			
			double brojPojavljivanjaD=(double)brojPojavljivanja;
			reduction=reduction+ent(vec)*brojPojavljivanja/(double)ex;
			IG[z]=IG[z]-reduction;
					
		}
		
		//cout << IG[z] << endl;
	}

	
	int ret=0;
	double maxSoFar=IG[0];
	
	for(int i=0;i<znacajkeIme.size();i++){
		if(IG[i]>maxSoFar){
			maxSoFar=IG[i];
			ret=i;
		}
	}
	
	return ret;
}

bool equalVecs(vector<vector<string> > v, string token){
	for(int i=0;i<v.size();i++){
		if(v[i][v[i].size()-1]!=token) return false;
	}
	return true;
}

void preorder(node *r){ //nije bas preorder, mogu s bilo cim doci do svih listova

	stek.push(r);

	while(!stek.empty()){ //ovo radim iterativno jer mi se èinilo da imam problema s kolièinom sistemskog stacka
		node *n=stek.top();
		stek.pop();
		if(n->isLeaf){
			vector<string> path;
			path.push_back(n->klasaIfLeaf);
			node *temp=n;
			int counter=0;
			
			while(temp->parent!=nullptr){
				temp=temp->parent;
				counter++;
			}
			
			while(n->parent!=nullptr){
				node *rc=n;
				n=n->parent;
				path.push_back(to_string(counter)+":"+n->znacajka+"="+rc->ZnacajkaDoOvdje);
				counter--;
			}		
			for(int i=path.size()-1;i>=0;i--) cout << path[i] << " ";
			cout << endl;
			
		}
		
		else{
			for(int i=0;i<n->subtrees.size();i++){
				node* nn=n->subtrees[i];
				stek.push(nn);
			}
		}
	}	
}

int getClassByName(string name){
	for(int i=0;i<klasaVrijednosti.size();i++){
		if(name==klasaVrijednosti[i]) return i;
	}
	return 0;
}







