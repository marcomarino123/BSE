#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <complex>
#include <cstdio>
#include <tuple>
#include <armadillo>
#include <omp.h>
#include <chrono>

using namespace std;
using namespace arma;

// declaring C function/libraries in the C++ code
extern "C"
{
// wrapper of the Fortran Lapack library into C
#include <stdio.h>
#include <omp.h>
#include <complex.h>
#include <lapacke.h>
}

///CONSTANT
const double minval = 1.0e-6;
const double pigreco = 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679;
const double electron_charge = 1.602176634;
const double vacuum_dielectric_constant = 8.8541878128;
const double conversionNmtoeV = 1.0e-12;
const double hbar = 6.582119569;
/// [hc]=eV*Ang 12.400 = Ry*Ang 911.38246268
const double hc = 911.38246268;
/// START DEFINITION DIFFERENT CLASSES

/// Crystal_Lattice class
class Crystal_Lattice
{
private:
	int number_atoms;
	double volume;
	mat atoms_coordinates;
	mat bravais_lattice{mat(3,3)};
	mat primitive_vectors{mat(3,3)};
public:
	Crystal_Lattice(string bravais_lattice_file_name, string atoms_coordinates_file_name,int number_atoms_tmp);
	vec pull_sitei_coordinates(int sitei);
	mat pull_bravais_lattice();
	mat pull_primitive_vectors();
	mat pull_atoms_coordinates();
	int pull_number_atoms();
	double pull_volume();
	void print();
	~Crystal_Lattice(){
		number_atoms=0;
		volume=0.0;
	};
};
int Crystal_Lattice::pull_number_atoms(){
	return number_atoms;
};
vec Crystal_Lattice::pull_sitei_coordinates(int sitei){
	return atoms_coordinates.col(sitei);
};
mat Crystal_Lattice::pull_bravais_lattice(){
	return bravais_lattice;
};
mat Crystal_Lattice::pull_atoms_coordinates(){
	return atoms_coordinates;
};
double Crystal_Lattice::pull_volume(){
	return volume;
};
mat Crystal_Lattice::pull_primitive_vectors(){
	return primitive_vectors;
};
Crystal_Lattice::Crystal_Lattice(string bravais_lattice_file_name,string atoms_coordinates_file_name,int number_atoms_tmp){
	ifstream bravais_lattice_file;
	bravais_lattice_file.open(bravais_lattice_file_name);
	ifstream atoms_coordinates_file;
	atoms_coordinates_file.open(atoms_coordinates_file_name);
	
	bravais_lattice_file.seekg(0);
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			bravais_lattice_file >> bravais_lattice(i, j);
	
	number_atoms=number_atoms_tmp;
	atoms_coordinates.set_size(3,number_atoms);
	atoms_coordinates_file.seekg(0);
	for (int i=0;i<number_atoms;i++)
		for (int j=0;j<3;j++)
			atoms_coordinates_file>>atoms_coordinates(j, i);
			
	volume = arma::det(bravais_lattice);

	vec b0(3);
	vec b1(3);
	vec b2(3);
	for(int i=0;i<3;i++){
		b0(i)=bravais_lattice(i,0);
		b1(i)=bravais_lattice(i,1);
		b2(i)=bravais_lattice(i,2);
	}
	primitive_vectors.col(0) = cross(b1,b2);
	primitive_vectors.col(1) = cross(b2,b0);
	primitive_vectors.col(2) = cross(b0,b1);
	double factor = 2 * pigreco / volume;

	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			primitive_vectors(i, j) = factor*primitive_vectors(i, j);
	
	bravais_lattice_file.close();
	atoms_coordinates_file.close();
};
void Crystal_Lattice::print()
{
	cout << "Bravais Lattice:" << endl;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
			cout << bravais_lattice(i, j) << " ";
		cout << endl;
	}
	cout << "Atoms Coordinates:" << endl;
	for (int i = 0; i < number_atoms; i++)
	{
		for (int j = 0; j < 3; j++)
			cout << atoms_coordinates(j, i) << " ";
		cout << endl;
	}
};
/// K_points class
/// it is possible to define a list of k points directly from the BZ or as an input
/// in the class K_points the points of FBZ are saved as k_points_list, while the points outside of the FBZ, defining the rest of the reciprocal lattice, are saved as g_points_list
class K_points
{
private:
	double spacing;
	int number_k_points_list;
	int dimension;
	mat primitive_vectors{mat(3,3)};
	mat k_points_list;
	vec shift{vec(3)}; 
	vec direction_cutting{vec(3)};
public:
	K_points(Crystal_Lattice *crystal_lattice,vec shift_tmp);
	void push_k_points_list_values(string k_points_list_file_name,int number_k_points_list_tmp);
	void push_k_points_list_values(double spacing_tmp,int dimension_tmp,vec direction_cutting_tmp);
	int pull_number_k_points_list();
	mat pull_k_points_list_values();
	mat pull_primitive_vectors();
	vec pull_shift();
	void print();
	~K_points(){
		spacing=0;
		number_k_points_list=0;
	};
};
K_points::K_points(Crystal_Lattice *crystal_lattice,vec shift_tmp){
	shift=shift_tmp;
	primitive_vectors=crystal_lattice->pull_primitive_vectors();
};
mat K_points::pull_primitive_vectors(){
	return primitive_vectors;
};
void K_points::push_k_points_list_values(string k_points_list_file_name,int number_k_points_list_tmp){
	number_k_points_list = number_k_points_list_tmp;
	cout<<"number points "<<number_k_points_list<<endl;
	ifstream k_points_list_file;
	k_points_list_file.open(k_points_list_file_name);
	k_points_list_file.seekg(0);
	k_points_list.set_size(3, number_k_points_list);
	int counting = 0;
	while (k_points_list_file.peek()!=EOF){
		if (counting<number_k_points_list)
		{
			k_points_list_file >> k_points_list(0, counting);
			k_points_list_file >> k_points_list(1, counting);
			k_points_list_file >> k_points_list(2, counting);
			counting = counting + 1;
		}
		else
			///to avoid the reading of blank rows			
			break;
	}
	k_points_list_file.close();
};
void K_points::push_k_points_list_values(double spacing_tmp,int dimension_tmp,vec direction_cutting_tmp){
	dimension=dimension_tmp;
	direction_cutting=direction_cutting_tmp;
	spacing=spacing_tmp;
	if(dimension==3){
		vec vec_number_k_points_list(3);
		for (int i = 0; i < 3; i++)
			vec_number_k_points_list(i) = int(sqrt(accu(primitive_vectors.col(i) % primitive_vectors.col(i))) / spacing);
		int limiti = int(vec_number_k_points_list(0));
		int limitj = int(vec_number_k_points_list(1));
		int limitk = int(vec_number_k_points_list(2));
		number_k_points_list = limiti * limitj * limitk;
		k_points_list.set_size(3,number_k_points_list);
		int counting = 0;
		for (int i = 0; i < limiti; i++)
			for (int j = 0; j < limitj; j++)
				for (int k = 0; k < limitk; k++){
					for (int r = 0; r < 3; r++)
						k_points_list(r, counting) = ((double)i / limiti) * (shift(r) + primitive_vectors(r, 0)) + ((double)j / limitj) * (shift(r) + primitive_vectors(r, 1)) + ((double)k / limitk) * (shift(r) + primitive_vectors(r, 2));
					counting = counting + 1;
				}
	}
	///TO IMPLEMENT OTHER CONDITIONS
};
vec K_points::pull_shift(){
	return shift;
};
mat K_points::pull_k_points_list_values(){
	return k_points_list;
};
int K_points::pull_number_k_points_list(){
	return number_k_points_list;
};
void K_points::print(){
	cout << "K points list" << endl;
	cout << number_k_points_list <<endl;
	for (int i = 0; i < number_k_points_list; i++)
	{
		cout << " ( ";
		for (int r = 0; r < 3; r++)
			cout << k_points_list(r, i) << " ";
		cout << " ) " << endl;
	}
	cout<<endl;
};
/// G_points class
/// the case one G point has to be properly implemented
class G_points
{
private:
	int number_g_points_list;
	vec number_g_points_direction;
	mat g_points_list;
	double cutoff_g_points_list;
	int dimension_g_points_list;
	vec direction_cutting{vec(3)};
	mat bravais_lattice{mat(3,3)};
	vec shift{vec(3)};
public:
	G_points(Crystal_Lattice *crystal_lattice,double cutoff_g_points_list_tmp,int dimension_g_points_list_tmp,vec direction_cutting_tmp,vec shift_tmp);
	mat pull_g_points_list_values();
	int pull_number_g_points_list();
	void print();
	~G_points(){
		number_g_points_list=0;
		cutoff_g_points_list=0.0;
		dimension_g_points_list=0;
	};
};
G_points::G_points(Crystal_Lattice *crystal_lattice,double cutoff_g_points_list_tmp,int dimension_g_points_list_tmp,vec direction_cutting_tmp,vec shift_tmp){
	dimension_g_points_list=dimension_g_points_list_tmp;
	direction_cutting=direction_cutting_tmp;
	cutoff_g_points_list=cutoff_g_points_list_tmp;
	shift=shift_tmp;

	mat primitive_vectors=crystal_lattice->pull_primitive_vectors();
	bravais_lattice=crystal_lattice->pull_bravais_lattice();
	cout<<"testing"<<endl;
	for(int i=0;i<3;i++)
		for(int j=0;j<3;j++)
			cout<<dot(bravais_lattice.col(j),primitive_vectors.col(i))/(2*pigreco)<<endl;

	double max_g_value=cutoff_g_points_list*1000/hc;

	cout<<"Calculating g values..."<<endl;
	if(cutoff_g_points_list!=0){
		if(dimension_g_points_list==3){
			number_g_points_direction.zeros(3);
			number_g_points_list=1;
			for(int i=0;i<3;i++){
				number_g_points_direction(i)=int(max_g_value/norm(primitive_vectors.col(i),2));
				number_g_points_list=number_g_points_list*(2*number_g_points_direction(i)+1);
			}
			g_points_list.set_size(3,number_g_points_list);
			//int counting=1;
			int counting=0;
			for (int i = -number_g_points_direction(0); i <= number_g_points_direction(0); i++)
				for (int j = -number_g_points_direction(1); j <= number_g_points_direction(1); j++)
					for (int k = -number_g_points_direction(2); k <= number_g_points_direction(2); k++)
					{
						//if ((i!=0)||(j!=0)||(k!=0)){
							for (int r = 0; r < 3; r++)
								g_points_list(r, counting) =  i * (shift(r) + primitive_vectors(r, 0)) + j * (shift(r) + primitive_vectors(r, 1)) + k * (shift(r) + primitive_vectors(r, 2));
							counting = counting + 1;
						//}
					}
		}else if(dimension_g_points_list==2){
			number_g_points_direction.zeros(2);
			mat reciprocal_plane_along; reciprocal_plane_along.zeros(3,2);
			int counting=0;
			number_g_points_list=1;
			for(int i=0;i<3;i++)
				if(direction_cutting(i)==1){
					reciprocal_plane_along.col(counting)=primitive_vectors.col(i);
					number_g_points_direction(counting)=int(max_g_value/norm(primitive_vectors.col(i),2));
					number_g_points_list=number_g_points_list*(2*number_g_points_direction(i)+1);
					cout<<number_g_points_direction(counting)<<endl;
					counting++;
				}
			g_points_list.set_size(3,number_g_points_list);
			//cout << number_g_points_list << endl;
			counting = 1;
			for (int i = -number_g_points_direction(0); i <= number_g_points_direction(0); i++)
				for (int j = -number_g_points_direction(1); j <= number_g_points_direction(1); j++){
					if((i!=0)||(j!=0)){
						for (int r = 0; r < 3; r++)
							g_points_list(r, counting) = i * (shift(r) + reciprocal_plane_along(r, 0)) + j * (shift(r) + reciprocal_plane_along(r, 1));
						//cout<<g_points_list.col(count)<<endl;
						counting = counting + 1;
					}
				}
		}
		/// TO IMPLEMENT OTHER CASE
	}else{
		number_g_points_list=1;
		g_points_list.zeros(3,number_g_points_list);
	}
};
mat G_points::pull_g_points_list_values(){
	return g_points_list;
};
int G_points::pull_number_g_points_list(){
	return number_g_points_list;
};
void G_points::print(){
	cout << "G points list "<<number_g_points_list << endl;
	for (int i = 0; i < number_g_points_list; i++){
		cout << " ( ";
		for (int r = 0; r < 3; r++)
			cout << i<<" "<< g_points_list(r, i) << " ";
		cout << " ) " << endl;
	}
};
/// Hamiltonian_TB class
class Hamiltonian_TB
{
private:
	int spinorial_calculation;
	int number_wannier_functions;
	int htb_basis_dimension;
	int number_atoms;
	int number_primitive_cells;
	vec weights_primitive_cells;
	mat positions_primitive_cells;
	field<cx_cube> hamiltonian;
	field<mat> wannier_centers;
	bool dynamic_shifting;
	double fermi_energy;
	double little_shift;
	double scissor_operator;
public:
	Hamiltonian_TB(){
		number_wannier_functions = 0;
		htb_basis_dimension = 0;
		spinorial_calculation = 0;
		fermi_energy = 0;
		number_primitive_cells = 0;
		dynamic_shifting = false;
	};
	/// reading hamiltonian from wannier90 output
	Hamiltonian_TB(string wannier90_hr_file_name,string wannier90_centers_file_name,double fermi_energy_tmp,int spinorial_calculation_tmp,int number_atoms_tmp,bool dynamic_shifting_tmp,double little_shift_tmp,double scissor_operator_tmp);
	field<cx_mat> FFT(vec k_point);
	tuple<mat, cx_mat> pull_ks_states(vec k_point);
	tuple<mat, cx_mat> pull_ks_states_subset(vec k_point,int number_valence_bands_selected,int number_conduction_bands_selected);
	field<cx_cube> pull_hamiltonian();
	int pull_htb_basis_dimension();
	int pull_number_wannier_functions();
	double pull_fermi_energy();
	void print_hamiltonian();
	void print_ks_states(vec k_point, int number_valence_bands_selected, int number_conduction_bands_selected);
	field<mat> pull_wannier_centers();
	void print();
	~Hamiltonian_TB(){
		number_wannier_functions = 0;
		htb_basis_dimension = 0;
		spinorial_calculation = 0;
		fermi_energy = 0;
		number_primitive_cells = 0;
		dynamic_shifting = false;
	};
};
Hamiltonian_TB::Hamiltonian_TB(string wannier90_hr_file_name,string wannier90_centers_file_name,double fermi_energy_tmp,int spinorial_calculation_tmp,int number_atoms_tmp,bool dynamic_shifting_tmp,double little_shift_tmp,double scissor_operator_tmp){
	cout<<"Be Carefull: if you are doing a collinear spin calculation, the number of Wannier functions in the two spin channels has to be the same!!"<<endl;
	fermi_energy=fermi_energy_tmp;
	number_atoms=number_atoms_tmp;
	spinorial_calculation=spinorial_calculation_tmp;
	dynamic_shifting=dynamic_shifting_tmp;
	scissor_operator=scissor_operator_tmp;
	ifstream wannier90_hr_file;
	ifstream wannier90_centers_file;
	wannier90_hr_file.open(wannier90_hr_file_name);
	wannier90_centers_file.open(wannier90_centers_file_name);

	cout<<"Reading Hamiltonian..."<<endl;
	int total_elements;
	string history_time;
	int counting_primitive_cells;
	int counting_positions;
	int l;	int m;
	double trashing_positions[3];
	string trashing_lines;
	double real_part;
	double imag_part;
	int spin_channel = 0;
	wannier90_hr_file.seekg(0);
	/// the Hamiltonians for the spinorial calculation = 1, should be one under the other(all the hr FILE (time included))
	while (wannier90_hr_file.peek() != EOF && spin_channel < 2){
		getline(wannier90_hr_file >> ws, history_time);
		wannier90_hr_file >> number_wannier_functions;
		wannier90_hr_file >> number_primitive_cells;
		cout<<"Number wannier functions "<<number_wannier_functions<<endl;
		cout<<"Number primitive cells "<<number_primitive_cells<<endl;
		if (spin_channel == 0){
			// initialization of the vriables
			weights_primitive_cells.set_size(number_primitive_cells);
			positions_primitive_cells.set_size(3, number_primitive_cells);
			if (spinorial_calculation == 1){
				/// two loops
				hamiltonian.set_size(2);
				hamiltonian(0).set_size(number_wannier_functions, number_wannier_functions, number_primitive_cells);
				hamiltonian(1).set_size(number_wannier_functions, number_wannier_functions, number_primitive_cells);
				htb_basis_dimension = number_wannier_functions * 2;
			}
			else{
				/// one loop
				hamiltonian.set_size(1);
				hamiltonian(0).set_size(number_wannier_functions, number_wannier_functions, number_primitive_cells);
				htb_basis_dimension = number_wannier_functions;
			}
		}
		total_elements = number_wannier_functions * number_wannier_functions * number_primitive_cells;
		//cout<<"Total elements "<<total_elements<<endl;
		counting_positions = 0;
		while (counting_positions < number_primitive_cells){
			wannier90_hr_file >> weights_primitive_cells(counting_positions);
			//cout<<counting_positions<<" "<<weights_primitive_cells(counting_positions)<<endl;
			counting_positions++;
		}
		counting_primitive_cells = 0;
		counting_positions = 0;
		/// the hamiltonian in the collinear case is diagonal in the spin channel
		while (counting_positions < total_elements){
			if (counting_positions == number_wannier_functions * number_wannier_functions * counting_primitive_cells)
			{
				wannier90_hr_file >> positions_primitive_cells(0, counting_primitive_cells) >> positions_primitive_cells(1, counting_primitive_cells) >> positions_primitive_cells(2, counting_primitive_cells);	
				counting_primitive_cells = counting_primitive_cells + 1;
			}
			else
				wannier90_hr_file >> trashing_positions[0] >> trashing_positions[1] >> trashing_positions[2];
			wannier90_hr_file >> l >> m;
			wannier90_hr_file >> real_part >> imag_part;

			hamiltonian(spin_channel)(l - 1, m - 1, counting_primitive_cells - 1).real(real_part);
			hamiltonian(spin_channel)(l - 1, m - 1, counting_primitive_cells - 1).imag(imag_part);
			counting_positions++;
		}
		///cout<<spin_channel<<" "<<l<<" "<<m<<" "<<real(hamiltonian(spin_channel).tube(l-1, m-1))<<" "<<imag(hamiltonian(spin_channel).tube(l-1, m-1))<<" "<<counting_positions<<" "<<total_elements<<" "<<counting_primitive_cells<<" "<<number_primitive_cells<<endl;
		///cout<<spinorial_calculation<<" "<<number_wannier_functions<<" "<<number_primitive_cells<<" "<<spin_channel<<" "<<endl;
		if (spinorial_calculation==1)
			spin_channel = spin_channel + 1;
		else
			spin_channel=2;
	}
	cout<<"Hamiltonian saved."<<endl;
	//if (wannier90_centers_file == NULL)
	//	throw std::invalid_argument("No Wannier90 Centers file!");
	//else
	cout<<"Reading Centers..."<<endl;
	char element_name;
	int number_lines;
	spin_channel=0;
	wannier90_centers_file.seekg(0);
	while (wannier90_centers_file.peek() != EOF && spin_channel<2){
		if (spin_channel == 0){
			// initialization of the variables
			if (spinorial_calculation == 1){
				/// two loops
				wannier_centers.set_size(2);
				wannier_centers(0).set_size(3,number_wannier_functions);
				wannier_centers(1).set_size(3,number_wannier_functions);
			}
			else{
				/// one loop
				wannier_centers.set_size(1);
				wannier_centers(0).set_size(3,number_wannier_functions);
			}
		}

		wannier90_centers_file >> number_lines;
		getline(wannier90_centers_file >> ws, history_time);
		counting_positions = 0;
		while (counting_positions < number_wannier_functions){
			wannier90_centers_file >> element_name >> wannier_centers(spin_channel)(0,counting_positions) >> wannier_centers(spin_channel)(1,counting_positions) >> wannier_centers(spin_channel)(2,counting_positions);
			//cout<<spin_channel<<" "<<counting_positions<<" "<<wannier_centers(spin_channel)(0,counting_positions)<<" "<<wannier_centers(spin_channel)(1,counting_positions)<<" "<<wannier_centers(spin_channel)(2,counting_positions)<<endl;
			counting_positions++;
		}
		counting_positions = 0;
		while (counting_positions < number_atoms){
			getline(wannier90_centers_file >> ws, trashing_lines);
			counting_positions++;
		}
		//cout<<spin_channel<<" "<<number_atoms<<endl;
		spin_channel++;
	}
	cout<<"Centers saved."<<endl;
	wannier90_hr_file.close();
	wannier90_centers_file.close();
};
field<cx_cube> Hamiltonian_TB::pull_hamiltonian(){
	return hamiltonian;
};
int Hamiltonian_TB::pull_htb_basis_dimension(){
	return htb_basis_dimension;
};
int Hamiltonian_TB::pull_number_wannier_functions(){
	return number_wannier_functions;
};
double Hamiltonian_TB::pull_fermi_energy(){
	return fermi_energy;
};
void Hamiltonian_TB::print(){
	cout<<"Printing Hamiltonian..."<<endl;
	int spin_counting = 0;
	while (spin_counting < 2){
		for (int i = 0; i < number_primitive_cells; i++)
			for (int q = 0; q < number_wannier_functions; q++)
				for (int s = 0; s < number_wannier_functions; s++){
					for (int r = 0; r < 3; r++)
						cout << positions_primitive_cells(r, i) << " ";
					cout << s << " " << q << " " << hamiltonian(spin_counting)(s, q, i) << endl;
				}
		cout<<"Printing Wannier Centers..."<<endl;
		for(int i=0;i<number_wannier_functions;i++){
			for (int r = 0; r < 3; r++)
				cout << wannier_centers(spin_counting)(r,i) <<" ";
			cout<<endl;
		}
		if (spinorial_calculation == 1)
			spin_counting = spin_counting + 1;
		else
			spin_counting = 2;
	}
};
field<mat> Hamiltonian_TB::pull_wannier_centers(){
	return wannier_centers;
};
field<cx_mat> Hamiltonian_TB::FFT(vec k_point){
	int flag_spin_channel = 0;
	int offset;
	
	vec temporary_cos(number_primitive_cells);
	vec temporary_sin(number_primitive_cells);
	vec real_part_hamiltonian(number_primitive_cells);
	vec imag_part_hamiltonian(number_primitive_cells);
	//#pragma omp parallel for 
	for (int r = 0; r < number_primitive_cells; r++){
		temporary_cos(r) = cos(accu(k_point % positions_primitive_cells.col(r)));
		temporary_sin(r) = sin(accu(k_point % positions_primitive_cells.col(r)));
		///cout<<r<<positions_primitive_cells.col(r)<<" "<<k_point<<" "<<temporary_cos(r)<<" "<<temporary_sin(r)<<endl;
	}
	if (spinorial_calculation == 1){
		field<cx_mat> fft_hamiltonian(2);
		fft_hamiltonian(0).zeros(number_wannier_functions,number_wannier_functions);
		fft_hamiltonian(1).zeros(number_wannier_functions,number_wannier_functions);
		while (flag_spin_channel < 2){
			offset = number_wannier_functions * flag_spin_channel;
			//#pragma omp parallel for collapse(2) private(real_part_hamiltonian,imag_part_hamiltonian)
			for (int l = 0; l < number_wannier_functions; l++)
				for (int m = 0; m < number_wannier_functions; m++){
					real_part_hamiltonian = real(hamiltonian(flag_spin_channel).tube(l, m));
					imag_part_hamiltonian = imag(hamiltonian(flag_spin_channel).tube(l, m));
					real_part_hamiltonian = real_part_hamiltonian%weights_primitive_cells;
					imag_part_hamiltonian = imag_part_hamiltonian%weights_primitive_cells;
					fft_hamiltonian(flag_spin_channel)(l, m).real(accu(real_part_hamiltonian % temporary_cos) - accu(imag_part_hamiltonian % temporary_sin));
					fft_hamiltonian(flag_spin_channel)(l, m).imag(accu(imag_part_hamiltonian % temporary_cos) + accu(real_part_hamiltonian % temporary_sin));
				}
			//cout<<flag_spin_channel<<" "<<fft_hamiltonian(flag_spin_channel)<<endl;
			flag_spin_channel++;
			
		}
		return fft_hamiltonian;
	}else{
		field<cx_mat> fft_hamiltonian(1);
		fft_hamiltonian(0).zeros(number_wannier_functions, number_wannier_functions);
		//#pragma omp parallel for collapse(2) private(real_part_hamiltonian,imag_part_hamiltonian)
		for (int l = 0; l < number_wannier_functions; l++)
		{
			for (int m = 0; m < number_wannier_functions; m++)
			{
				real_part_hamiltonian = real(hamiltonian(0).tube(l, m));
				imag_part_hamiltonian = imag(hamiltonian(0).tube(l, m));
				real_part_hamiltonian = real_part_hamiltonian%weights_primitive_cells;
				imag_part_hamiltonian = imag_part_hamiltonian%weights_primitive_cells;
				fft_hamiltonian(0)(l, m).real(accu(real_part_hamiltonian % temporary_cos) - accu(imag_part_hamiltonian % temporary_sin));
				fft_hamiltonian(0)(l, m).imag(accu(imag_part_hamiltonian % temporary_cos) + accu(real_part_hamiltonian % temporary_sin));
			}
		}
		fft_hamiltonian(0)=fft_hamiltonian(0);
		return fft_hamiltonian;
	}	
};
tuple<mat, cx_mat> Hamiltonian_TB::pull_ks_states(vec k_point){
	/// the eigenvalues are saved into a two component element, in order to make the code more general
	mat ks_eigenvalues_spinor(2, number_wannier_functions, fill::zeros);
	cx_mat ks_eigenvectors_spinor(htb_basis_dimension, number_wannier_functions, fill::zeros);
	
	//cout<<"fourier transform starting"<<endl;
	field<cx_mat> fft_hamiltonian = FFT(k_point);
	///cout<<"fourier transform ending"<<endl;

	if (spinorial_calculation == 1){
		cx_mat hamiltonian_up(number_wannier_functions, number_wannier_functions);
		cx_mat hamiltonian_down(number_wannier_functions, number_wannier_functions);
		hamiltonian_up = fft_hamiltonian(0);
		hamiltonian_down = fft_hamiltonian(1);
		//cout<<"diagonanlization starting"<<endl;
		vec eigenvalues_up(number_wannier_functions);
		cx_mat eigenvectors_up(number_wannier_functions,number_wannier_functions);
		vec eigenvalues_down(number_wannier_functions);
		cx_mat eigenvectors_down(number_wannier_functions,number_wannier_functions);
		//ARMADILLO DIAGONALIZATION ROUTINE BADLY FAILING; USING THE LAPACKE DIAGONALIZATION ROUTINE, INSTEAD
		//eig_gen(eigenvalues_up,eigenvectors_up, hamiltonian_up);
		//eig_gen(eigenvalues_down,eigenvectors_down, hamiltonian_down);
		lapack_complex_double *temporary_up; temporary_up = (lapack_complex_double *)malloc(number_wannier_functions*number_wannier_functions*sizeof(lapack_complex_double));
		lapack_complex_double *temporary_down; temporary_down = (lapack_complex_double *)malloc(number_wannier_functions*number_wannier_functions*sizeof(lapack_complex_double));

		#pragma omp parallel for collapse(2)
		for(int i=0;i<number_wannier_functions;i++)
			for(int j=0;j<number_wannier_functions;j++){
				temporary_up[i*number_wannier_functions+j]=real(hamiltonian_up(i,j))+_Complex_I*imag(hamiltonian_up(i,j));
				temporary_down[i*number_wannier_functions+j]=real(hamiltonian_down(i,j))+_Complex_I*imag(hamiltonian_down(i,j));
			}
	
		int N=number_wannier_functions;
		int LDA=number_wannier_functions;
		int matrix_layout = 101;
		int INFO;
		double *w_up; complex<double> **u_up;
		double *w_down; complex<double> **u_down;
		char JOBZ = 'V'; char UPLO = 'L';
		//// saving all the eigenvalues
		w_up = (double *)malloc(number_wannier_functions * sizeof(double));
		w_down = (double *)malloc(number_wannier_functions * sizeof(double));
		INFO = LAPACKE_zheev(matrix_layout, JOBZ, UPLO, N, temporary_up, LDA, w_up);
		INFO = LAPACKE_zheev(matrix_layout, JOBZ, UPLO, N, temporary_down, LDA, w_down);
		#pragma omp parallel for collapse(2)
		for(int i=0;i<number_wannier_functions;i++){
			for(int j=0;j<number_wannier_functions;j++){
				eigenvectors_up(i,j)=lapack_complex_double_real(temporary_up[i*number_wannier_functions+j])+_Complex_I*lapack_complex_double_imag(temporary_up[i*number_wannier_functions+j]);
				eigenvectors_down(i,j)=lapack_complex_double_real(temporary_down[i*number_wannier_functions+j])+_Complex_I*lapack_complex_double_imag(temporary_down[i*number_wannier_functions+j]);
			}
		}
		for(int i=0;i<number_wannier_functions;i++){
			eigenvalues_up(i)=w_up[i];
			eigenvalues_down(i)=w_down[i];
		}
		uvec ordering_up = sort_index(real(eigenvalues_up));
		uvec ordering_down = sort_index(real(eigenvalues_down));
		/// in the case of spinorial_calculation=1 combining the two components of spin into a single spinor
		/// saving the ordered eigenvectors in the matrix ks_eigenvectors_spinor
		for (int i = 0; i < number_wannier_functions; i++){
			for (int j = 0; j < htb_basis_dimension; j++){
				if (j < number_wannier_functions)
					ks_eigenvectors_spinor(j, i) = eigenvectors_up(j, ordering_up(i));
				else
					ks_eigenvectors_spinor(j, i) = eigenvectors_down(j - number_wannier_functions, ordering_down(i));
			}
			ks_eigenvectors_spinor.col(i) = ks_eigenvectors_spinor.col(i) / norm(ks_eigenvectors_spinor.col(i), 2);
			for (int r = 0; r < 2; r++)
				ks_eigenvalues_spinor(r, i) = (1 - r) * real(eigenvalues_up(ordering_up(i))) + r * real(eigenvalues_down(ordering_down(i)));
		}
		//cout<<"diagonanlization ending"<<endl;
		return {ks_eigenvalues_spinor, ks_eigenvectors_spinor};
	
	}else{
		int N=number_wannier_functions;
		int LDA=number_wannier_functions;
		int matrix_layout = 101;
		int INFO;
		double *w; complex<double> **u;
		char JOBZ = 'V'; char UPLO = 'L';

		//// saving all the eigenvalues
		w = (double *)malloc(number_wannier_functions * sizeof(double));
		
		lapack_complex_double *temporary; temporary = (lapack_complex_double *)malloc(number_wannier_functions*number_wannier_functions*sizeof(lapack_complex_double));
		#pragma omp parallel for collapse(2)
		for(int i=0;i<number_wannier_functions;i++)
			for(int j=0;j<number_wannier_functions;j++)
				temporary[i*number_wannier_functions+j]=real(fft_hamiltonian(0)(i,j))+_Complex_I*imag(fft_hamiltonian(0)(i,j));
		
		INFO = LAPACKE_zheev(matrix_layout, JOBZ, UPLO, N, temporary, LDA, w);

		cx_vec eigenvalues(number_wannier_functions);
		cx_mat eigenvectors(number_wannier_functions,number_wannier_functions);

		#pragma omp parallel for collapse(2)
		for(int i=0;i<number_wannier_functions;i++)
			for(int j=0;j<number_wannier_functions;j++)
				eigenvectors(i,j)=lapack_complex_double_real(temporary[i*number_wannier_functions+j])+_Complex_I*lapack_complex_double_imag(temporary[i*number_wannier_functions+j]);

		for(int i=0;i<number_wannier_functions;i++)
			eigenvalues(i)=w[i];

		free(w); free(temporary);

		uvec ordering = sort_index(real(eigenvalues));

		/// in the case of spinorial_calculation=1 combining the two components of spin into a single spinor
		/// saving the ordered eigenvectors in the matrix ks_eigenvectors_spinor
		#pragma omp parallel for collapse(2)
		for (int i = 0; i < number_wannier_functions; i++)
			for (int j = 0; j < htb_basis_dimension; j++)
				ks_eigenvectors_spinor(j, i) = eigenvectors(j, ordering(i))/norm(eigenvectors.col(ordering(i)), 2);

		for (int i = 0; i < number_wannier_functions; i++){
			ks_eigenvalues_spinor(0,i)=real(eigenvalues(ordering(i)));
			ks_eigenvalues_spinor(1,i)=real(eigenvalues(ordering(i)));
		}
		return {ks_eigenvalues_spinor, ks_eigenvectors_spinor};
	}
};
tuple<mat, cx_mat> Hamiltonian_TB::pull_ks_states_subset(vec k_point,int number_valence_bands_selected,int number_conduction_bands_selected){
	int number_valence_bands = 0;
	int number_conduction_bands = 0;
	int dimensions_subspace = number_conduction_bands_selected + number_valence_bands_selected;
	vec spinor_scissor_operator(2);
	spinor_scissor_operator(0)=scissor_operator;
	spinor_scissor_operator(1)=scissor_operator;
	
	tuple<mat, cx_mat> ks_states= pull_ks_states(k_point);
	mat ks_eigenvalues = get<0>(ks_states); 
	cx_mat ks_eigenvectors = get<1>(ks_states);

	/// distinguishing between valence and conduction states
	for (int i = 0; i < number_wannier_functions; i++){
		///cout<<ks_eigenvalues(0, i)<<" "<<ks_eigenvalues(1, i)<<endl; 
		if (ks_eigenvalues(0, i)<=fermi_energy && ks_eigenvalues(1, i)<=fermi_energy)
			number_valence_bands++;
		else
			number_conduction_bands++;
	}
	//cout<<"Number valence bands "<<number_valence_bands<<" Number conduction bands "<<number_conduction_bands<<endl;
	
	/// in a single matrix: first are written valence states, than (at higher rows) conduction states
	mat ks_eigenvalues_subset(2, dimensions_subspace);
	cx_mat ks_eigenvectors_subset(htb_basis_dimension, dimensions_subspace);
	for (int i = 0; i < dimensions_subspace; i++){
		if (i < number_valence_bands_selected){
			ks_eigenvectors_subset.col(i) = ks_eigenvectors.col((number_valence_bands - 1) - i);
			ks_eigenvalues_subset.col(i) = ks_eigenvalues.col((number_valence_bands - 1) - i);
		}else{
			ks_eigenvectors_subset.col(i) = ks_eigenvectors.col(number_valence_bands + (i - number_valence_bands_selected));
			ks_eigenvalues_subset.col(i) = ks_eigenvalues.col(number_valence_bands + (i - number_valence_bands_selected))+ spinor_scissor_operator;
		}
	}
	return {ks_eigenvalues_subset, ks_eigenvectors_subset};
};
void Hamiltonian_TB::print_ks_states(vec k_point, int number_valence_bands_selected, int number_conduction_bands_selected){
	tuple<mat, cx_mat> results_htb;
	tuple<mat, cx_mat> results_htb_subset;
	cout<<"Extraction all ks states:"<<endl;
	results_htb=pull_ks_states(k_point);
	cout<<"Extraction subset ks states"<<endl;
	results_htb_subset=pull_ks_states_subset(k_point,number_valence_bands_selected,number_conduction_bands_selected);
	mat eigenvalues=get<0>(results_htb);
	cx_mat eigenvectors=get<1>(results_htb);
	mat eigenvalues_subset=get<0>(results_htb_subset);
	cx_mat eigenvectors_subset=get<1>(results_htb_subset);
	////In the case of spinorial_calculation=1, the spinor components of each wannier function are one afte the other
	////i.e. wannier function 0 : spin up component columnt 0, spin down component column 1...and so on...
	/////moreover, in the case of ks_subset the valence states are written before the conduction states
	cout<<"All bands"<<endl;
	for (int i = 0; i < number_wannier_functions; i++){
		printf("%d	%.5f %.5f\n", i, eigenvalues(0, i), eigenvalues(1, i));
		//for (int j = 0; j < htb_basis_dimension; j++)
		//	printf("(%.5f,%.5f)", real(eigenvectors(j, i)), imag(eigenvectors(j, i)));
		cout << endl;
	}
	cout << "Only subset" << endl;
	for (int i = 0; i < number_valence_bands_selected + number_conduction_bands_selected; i++){
		printf("%d	%.5f %.5f\n", i, eigenvalues_subset(0, i), eigenvalues_subset(1, i));
		//for (int j = 0; j < htb_basis_dimension; j++)
			//printf("(%.5f,%.5f)", real(eigenvectors_subset(j, i)), imag(eigenvectors_subset(j, i)));
		cout << endl;
	}
};

// Coulomb_Potential class
class Coulomb_Potential
{
private:
	double minimum_k_point_modulus;
	mat primitive_vectors{mat(3,3)};
	vec direction_cutting{vec(3)};
	int dimension_potential;
	K_points *k_points;
	G_points *g_points;
	double volume_cell;
public:
	Coulomb_Potential(K_points* k_points_tmp,G_points* g_points_tmp,double minimum_k_point_modulus_tmp,int dimension_potential_tmp,vec direction_cutting_tmp,double volume_tmp);
	double pull(vec k_point);
	double pull_volume();
	void print();
	void print_profile(int number_k_points,double max_radius,string file_coulomb_potential_name, int direction_profile_xyz);
	///~Coulomb_Potential(){
	///	k_points=NULL;
	///	g_points=NULL;
	///};
};
Coulomb_Potential::Coulomb_Potential(K_points* k_points_tmp,G_points* g_points_tmp,double minimum_k_point_modulus_tmp,int dimension_potential_tmp,vec direction_cutting_tmp,double volume_tmp){
	k_points = k_points_tmp;
	g_points = g_points_tmp;
	minimum_k_point_modulus = minimum_k_point_modulus_tmp;
	dimension_potential = dimension_potential_tmp;
	///direction of the bravais lattice along whic the cut is considered (1 is cut and 0 is no-cut)
	direction_cutting = direction_cutting_tmp;
	primitive_vectors = k_points->pull_primitive_vectors();
	volume_cell=volume_tmp;
};
double Coulomb_Potential::pull_volume(){
	return volume_cell;
};
void Coulomb_Potential::print(){
	cout<<"Minimum k point modulus: "<<minimum_k_point_modulus<<endl;
	cout<<"Primitive vectors: "<<endl;
	cout<<primitive_vectors<<endl;
	cout<<"Direction cutting "<<endl;
	cout<<direction_cutting<<endl;
	cout<<"Dimension potential: "<<dimension_potential<<endl;
	cout<<"K points address: "<<k_points<<endl;
};
void Coulomb_Potential:: print_profile(int number_k_points,double max_k_point,string file_coulomb_potential_name, int direction_profile_xyz){
	vec k_point(3,fill::zeros);
	ofstream file_coulomb_potential;
	file_coulomb_potential.open(file_coulomb_potential_name);
	double coulomb;
	for(int i=0;i<number_k_points;i++){
		k_point(direction_profile_xyz)=(double(i)/double(number_k_points))*max_k_point;
		coulomb=pull(k_point);
		file_coulomb_potential<<i<<" "<<k_point(direction_profile_xyz)<<" "<<coulomb<<endl;
		k_point(direction_profile_xyz)=0.0;
	}
	file_coulomb_potential.close();
};
double Coulomb_Potential::pull(vec k_point){
	/// the volume of the cell is in angstrom
	/// the momentum k is in angstrom^-1
	double coulomb_potential;

	/// a cutoff is introduced in order to avoid any divergence in k=0
	double modulus_k_point;
	modulus_k_point= accu(k_point%k_point);
	if(dimension_potential==3){
		if (modulus_k_point < minimum_k_point_modulus)
			coulomb_potential = 0;
		else
			coulomb_potential = conversionNmtoeV*4*pigreco/modulus_k_point;
	}else if(dimension_potential==2){
		vec primitive_along(3);
		for(int i=0;i<3;i++)
			if(direction_cutting(i)==0){
				primitive_along=primitive_vectors.col(i);
			}
		vec k_point_orthogonal(3,fill::zeros);
		vec k_point_along(3,fill::zeros);
		vec unity(3,fill::ones);
		k_point_along=(primitive_along%k_point)/norm(primitive_along);
		k_point_orthogonal=k_point-k_point_along;
		double c1=norm(k_point_along)/norm(k_point_orthogonal);
		double c2=(norm(primitive_along)/2)*norm(k_point_orthogonal);
		double c3=(norm(primitive_along)/2)*norm(k_point_along);
		if (modulus_k_point < minimum_k_point_modulus)
			coulomb_potential=0;
		else{
			coulomb_potential =  4*pigreco / (pow(modulus_k_point, 2));
			coulomb_potential = coulomb_potential * (1-exp(-c2)*(c1*sin(c3)-cos(c3)));
		}
	}
	///TO IMPLEMENT THIS CASE AS WELL
	return coulomb_potential;
};

/// Generalized dipoles elements
///rho_{n1,n2,k1-p,k2-q}(excitonic_momentum,G)=\bra{n1k1-p}e^{i(excitonic_momentum+G)r\ket{n2k2-q}
class Dipole_Elements
{
private:
	int number_k_points_list;
	int number_g_points_list;
	mat k_points_list;
	mat g_points_list;
	int htb_basis_dimension;
	int spin_htb_basis_dimension;
	int number_wannier_centers;
	field<mat> wannier_centers;
	int number_valence_bands;
	int spin_number_valence_bands;
	int number_conduction_bands;
	int spin_number_conduction_bands;
	int number_valence_plus_conduction;
	int spin_number_valence_plus_conduction;
	Hamiltonian_TB *hamiltonian_tb;
	int spinorial_calculation;
	umat pair_k_points;
public:
	Dipole_Elements(int number_k_points_list_tmp,mat k_points_list_tmp, int number_g_points_list_tmp, mat g_points_list_tmp,int number_wannier_centers_tmp,int number_valence_bands_selected_tmp,int number_conduction_bands_selected_tmp, Hamiltonian_TB *hamiltonian_tb_tmp,int spinorial_calculation_tmp);
	cx_mat function_building_exponential_factor(vec excitonic_momentum);
	///rho_{n1,n2,k1-p,k2-q}(excitonic_momentum,G)=\bra{n1k1-p}e^{i(excitonic_momentum+G)r\ket{n2k2-q}
	tuple<cx_mat,cx_mat> pull_values(vec excitonic_momentum,vec q_parameter,vec p_parameter,int diagonal_k);
	tuple<cx_mat,cx_mat> pull_diagonal_k(mat energies,cx_mat rho);
	cx_mat pull_reduced_values_vc(cx_mat rho,int diagonal_k);
	cx_mat pull_reduced_values_cv(cx_mat rho,int diagonal_k);
	cx_mat pull_reduced_values_cc_vv(cx_mat rho,int conduction_or_valence,int diagonal_k);
	////term 0: total, term 1: vc, term 2: cc term 3: vv
	void print(vec excitonic_momentum,vec q_parameter,int which_term,int diagonal_k);
};
Dipole_Elements::Dipole_Elements(int number_k_points_list_tmp, mat k_points_list_tmp, int number_g_points_list_tmp, mat g_points_list_tmp, int number_wannier_centers_tmp, int number_valence_bands_tmp, int number_conduction_bands_tmp, Hamiltonian_TB *hamiltonian_tb_tmp,int spinorial_calculation_tmp){
	number_g_points_list=number_g_points_list_tmp;
	number_conduction_bands=number_conduction_bands_tmp;
	number_valence_bands=number_valence_bands_tmp;
	number_k_points_list=number_k_points_list_tmp;
	number_wannier_centers=number_wannier_centers_tmp;
	number_valence_plus_conduction=number_valence_bands+number_conduction_bands;
	k_points_list=k_points_list_tmp;
	g_points_list=g_points_list_tmp;
	hamiltonian_tb=hamiltonian_tb_tmp;
	spinorial_calculation=spinorial_calculation_tmp;
	htb_basis_dimension=hamiltonian_tb->pull_htb_basis_dimension();
	spin_htb_basis_dimension=htb_basis_dimension/(spinorial_calculation+1);
	spin_number_valence_plus_conduction=(spinorial_calculation+1)*number_valence_plus_conduction;
	spin_number_valence_bands=(spinorial_calculation+1)*number_valence_bands;
	if (spinorial_calculation == 1){
		wannier_centers.set_size(2);
		wannier_centers(0).set_size(3,number_wannier_centers);
		wannier_centers(1).set_size(3,number_wannier_centers);
	}else{
		wannier_centers.set_size(1);
		wannier_centers(0).set_size(3,number_wannier_centers);
	}
	wannier_centers=hamiltonian_tb->pull_wannier_centers();
	
	///pairs of k points
	int number_k_point=0;
	pair_k_points.set_size(number_k_points_list*number_k_points_list,2);
	for(int i=0;i<number_k_points_list;i++)
		for(int j=0;j<number_k_points_list;j++){
			pair_k_points(number_k_point,0)=i;
			pair_k_points(number_k_point,1)=j;
			number_k_point+=1;
		}
};
cx_mat Dipole_Elements::function_building_exponential_factor(vec excitonic_momentum){
	cx_mat exponential_factor_tmp(htb_basis_dimension,number_g_points_list);
	#pragma omp parallel for collapse(3)
	for(int spin_channel=0;spin_channel<(spinorial_calculation+1);spin_channel++)
		for(int g=0; g<number_g_points_list; g++)
			for(int i=0; i<spin_htb_basis_dimension; i++){
				exponential_factor_tmp(spin_channel*spin_htb_basis_dimension+i,g).real(cos(accu((wannier_centers(spin_channel)).col(i)%(g_points_list.col(g)+excitonic_momentum))));
				exponential_factor_tmp(spin_channel*spin_htb_basis_dimension+i,g).imag(sin(accu((wannier_centers(spin_channel)).col(i)%(g_points_list.col(g)+excitonic_momentum))));
			}
	return exponential_factor_tmp;
};
///diagonal_k ---> rho_{n1,n2,k1-p,k2-q}(excitonic_momentum,G)-->rho_{n1,n2,k1-q,k1-p}(excitonic_momentum,G)
tuple<cx_mat,cx_mat> Dipole_Elements::pull_values(vec excitonic_momentum,vec q_parameter,vec p_parameter,int diagonal_k){
	cx_mat exponential_factor=function_building_exponential_factor(excitonic_momentum);
	tuple<mat,cx_mat> ks_states_k_point;
	tuple<mat,cx_mat> ks_states_k_point_q;
	cx_mat ks_state; 
	cx_mat ks_state_q; 
	mat ks_energy; 
	mat ks_energy_q;
	int effective_number_k_points_list;
	if(diagonal_k==0)
		effective_number_k_points_list=number_k_points_list*number_k_points_list;
	else
		effective_number_k_points_list=number_k_points_list;
	////this is the heaviest but also the fastest solution (to use cx_cub for left state instead of cx_mat)
	cx_cube ks_state_right(htb_basis_dimension,number_valence_plus_conduction*effective_number_k_points_list,number_g_points_list,fill::ones);
	cx_cube ks_state_left(htb_basis_dimension,number_valence_plus_conduction*effective_number_k_points_list,number_g_points_list,fill::ones);
	cx_mat rho(spin_number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list,number_g_points_list);
	cx_mat energies((spinorial_calculation+1),effective_number_k_points_list*number_valence_plus_conduction*number_valence_plus_conduction);
	///adding exponential term e^{i(k+G)r} to the right states
	///e_{gl}k_{gm} -> l_{g(l,m)}
	#pragma omp parallel for private(ks_states_k_point,ks_states_k_point_q,ks_state,ks_state_q,ks_energy,ks_energy_q)
	for(int i=0;i<effective_number_k_points_list;i++){
			ks_states_k_point = hamiltonian_tb->pull_ks_states_subset(k_points_list.col(int(pair_k_points(i,0)+pair_k_points(i,1)*diagonal_k))-q_parameter,number_valence_bands,number_conduction_bands);
			ks_states_k_point_q = hamiltonian_tb->pull_ks_states_subset(k_points_list.col(pair_k_points(i,1))-p_parameter,number_valence_bands,number_conduction_bands);
			ks_state=get<1>(ks_states_k_point); 
			ks_state_q=get<1>(ks_states_k_point_q);
			ks_energy=get<0>(ks_states_k_point); 
			ks_energy_q=get<0>(ks_states_k_point_q);
			for(int spin_channel=0;spin_channel<(spinorial_calculation+1);spin_channel++){
				for(int m=0;m<number_valence_plus_conduction;m++){
					for(int g=0;g<number_g_points_list;g++){
						ks_state_right.subcube(spin_channel*spin_htb_basis_dimension,m*effective_number_k_points_list+pair_k_points(i,0)*number_k_points_list+pair_k_points(i,1),g,(spin_channel+1)*spin_htb_basis_dimension-1,m*effective_number_k_points_list+pair_k_points(i,0)*number_k_points_list+pair_k_points(i,1),g)=
							exponential_factor.submat(spin_channel*spin_htb_basis_dimension,g,(spin_channel+1)*spin_htb_basis_dimension-1,g)%ks_state_q.submat(spin_channel*spin_htb_basis_dimension,m,(spin_channel+1)*spin_htb_basis_dimension-1,m);
						ks_state_left.subcube(spin_channel*spin_htb_basis_dimension,m*effective_number_k_points_list+pair_k_points(i,0)*number_k_points_list+pair_k_points(i,1),g,(spin_channel+1)*spin_htb_basis_dimension-1,m*effective_number_k_points_list+pair_k_points(i,0)*number_k_points_list+pair_k_points(i,1),g)=
							ks_state.submat(spin_channel*spin_htb_basis_dimension,m,(spin_channel+1)*spin_htb_basis_dimension-1,m);
					}
					for(int n=0;n<number_valence_plus_conduction;n++)
						energies(spin_channel,m*number_valence_plus_conduction*effective_number_k_points_list+n*effective_number_k_points_list+pair_k_points(i,0)*number_k_points_list+pair_k_points(i,1)).real(ks_energy(spin_channel,m)-ks_energy_q(spin_channel,n));
				}
			}
	}

	cx_mat temporary_matrix(effective_number_k_points_list,number_g_points_list,fill::zeros);
	for(int spin_channel=0;spin_channel<(spinorial_calculation+1);spin_channel++){
		#pragma omp parallel for collapse(2) private(temporary_matrix)
		for(int m=0;m<number_valence_plus_conduction;m++)
			for(int n=0;n<number_valence_plus_conduction;n++){
				temporary_matrix=sum(conj(ks_state_left.subcube(spin_channel*spin_htb_basis_dimension,m*effective_number_k_points_list,0,(spin_channel+1)*spin_htb_basis_dimension-1,(m+1)*effective_number_k_points_list-1,number_g_points_list-1))%
						ks_state_right.subcube(spin_channel*spin_htb_basis_dimension,n*effective_number_k_points_list,0,(spin_channel+1)*spin_htb_basis_dimension-1,(n+1)*effective_number_k_points_list-1,number_g_points_list-1),0);
				rho.submat(spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+m*number_valence_plus_conduction*effective_number_k_points_list+n*effective_number_k_points_list,0,spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+m*number_valence_plus_conduction*effective_number_k_points_list+n*effective_number_k_points_list+effective_number_k_points_list-1,number_g_points_list-1)=temporary_matrix;
			}
	}
	if(diagonal_k==1){
		///extracting energy differences Conduction - Valence
		cx_mat energies_vc((spinorial_calculation+1),number_conduction_bands*number_valence_bands*number_k_points_list);
		for(int spin_channel=0;spin_channel<(spinorial_calculation+1);spin_channel++)
			for(int v=0;v<number_valence_bands;v++)
				for(int c=0;c<number_conduction_bands;c++)
					for(int k=0;k<number_k_points_list;k++)
						energies_vc(spin_channel,c*number_valence_bands*number_k_points_list+v*number_k_points_list+k)=energies(spin_channel,v*number_valence_plus_conduction*number_k_points_list+(number_valence_bands+c)*number_k_points_list+k);
		return{energies_vc,rho};
	}else
		return {energies,rho};	
};
cx_mat Dipole_Elements::pull_reduced_values_vc(cx_mat rho, int diagonal_k){
	int effective_number_k_points_list;
	if(diagonal_k==0)
		effective_number_k_points_list=number_k_points_list*number_k_points_list;
	else
		effective_number_k_points_list=number_k_points_list;

	spin_number_valence_bands=(spinorial_calculation+1)*number_valence_bands;
	cx_mat rho_reduced(spin_number_valence_bands*number_conduction_bands*effective_number_k_points_list,number_g_points_list);
	//rho_reduced = rho(vc)
	#pragma omp parallel for collapse(3)
	for(int spin_channel=0;spin_channel<(spinorial_calculation+1);spin_channel++)
		for(int m=0;m<number_conduction_bands;m++)
			for(int n=0;n<number_valence_bands;n++)
				rho_reduced.submat(spin_channel*number_valence_bands*number_conduction_bands*effective_number_k_points_list+m*number_valence_bands*effective_number_k_points_list+n*effective_number_k_points_list,0,spin_channel*number_valence_bands*number_conduction_bands*effective_number_k_points_list+m*number_valence_bands*effective_number_k_points_list+(n+1)*effective_number_k_points_list-1,number_g_points_list-1)=
					rho.submat(spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+n*number_valence_plus_conduction*effective_number_k_points_list+(number_valence_bands+m)*effective_number_k_points_list,0,spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+n*number_valence_plus_conduction*effective_number_k_points_list+(number_valence_bands+m+1)*effective_number_k_points_list-1,number_g_points_list-1);
	return rho_reduced;
};
cx_mat Dipole_Elements::pull_reduced_values_cv(cx_mat rho, int diagonal_k){
	int effective_number_k_points_list;
	if(diagonal_k==0)
		effective_number_k_points_list=number_k_points_list*number_k_points_list;
	else
		effective_number_k_points_list=number_k_points_list;

	spin_number_valence_bands=(spinorial_calculation+1)*number_valence_bands;
	cx_mat rho_reduced (spin_number_valence_bands*number_conduction_bands*effective_number_k_points_list,number_g_points_list);
	//rho_reduced = rho(vc)
	///cout<<"dimension: "<<size(rho)<<endl;
	#pragma omp parallel for collapse(2)
	for(int spin_channel=0;spin_channel<(spinorial_calculation+1);spin_channel++)
		for(int m=0;m<number_conduction_bands;m++)
			rho_reduced.submat(spin_channel*number_conduction_bands*number_valence_bands*effective_number_k_points_list+m*number_valence_bands*effective_number_k_points_list,0,spin_channel*number_conduction_bands*number_valence_bands*effective_number_k_points_list+(m+1)*number_valence_bands*effective_number_k_points_list-1,number_g_points_list-1)=
				rho.submat(spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+(m+number_valence_bands)*number_valence_plus_conduction*effective_number_k_points_list,0,spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+(m+number_valence_bands)*number_valence_plus_conduction*effective_number_k_points_list+number_valence_bands*effective_number_k_points_list-1,number_g_points_list-1);
	return rho_reduced;
};
cx_mat Dipole_Elements:: pull_reduced_values_cc_vv(cx_mat rho, int conduction_or_valence, int diagonal_k){
	int effective_number_k_points_list;
	if(diagonal_k==0)
		effective_number_k_points_list=number_k_points_list*number_k_points_list;
	else
		effective_number_k_points_list=number_k_points_list;

	if(conduction_or_valence==0){
		spin_number_conduction_bands=(spinorial_calculation+1)*number_conduction_bands;
		cx_mat rho_reduced(spin_number_conduction_bands*number_conduction_bands*effective_number_k_points_list,number_g_points_list);
		//rho_reducde = rho(cc)
		for(int spin_channel=0;spin_channel<(spinorial_calculation+1);spin_channel++)
			for(int m=0;m<number_conduction_bands;m++)
				rho_reduced.submat(spin_channel*number_conduction_bands*number_conduction_bands*effective_number_k_points_list+m*number_conduction_bands*effective_number_k_points_list,0,spin_channel*number_conduction_bands*number_conduction_bands*effective_number_k_points_list+m*number_conduction_bands*effective_number_k_points_list+number_conduction_bands*effective_number_k_points_list-1,number_g_points_list-1)=
					rho.submat(spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+(number_valence_bands+m)*number_valence_plus_conduction*effective_number_k_points_list+number_valence_bands*effective_number_k_points_list,0,spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+(number_valence_bands+m)*number_valence_plus_conduction*effective_number_k_points_list+number_valence_plus_conduction*effective_number_k_points_list-1,number_g_points_list-1);
		return rho_reduced;
	}else{
		spin_number_valence_bands=(spinorial_calculation+1)*number_valence_bands;
		cx_mat rho_reduced(spin_number_valence_bands*number_valence_bands*effective_number_k_points_list,number_g_points_list);
		//rho_reducde = rho(vv)
		for(int spin_channel=0;spin_channel<(spinorial_calculation+1);spin_channel++)
			for(int m=0;m<number_valence_bands;m++)
				rho_reduced.submat(spin_channel*number_valence_bands*number_valence_bands*effective_number_k_points_list+m*number_valence_bands*effective_number_k_points_list,0,spin_channel*number_valence_bands*number_valence_bands*effective_number_k_points_list+m*number_valence_bands*effective_number_k_points_list+number_valence_bands*effective_number_k_points_list-1,number_g_points_list-1)=
					rho.submat(spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+m*number_valence_plus_conduction*effective_number_k_points_list,0,spin_channel*number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list+m*number_valence_plus_conduction*effective_number_k_points_list+number_valence_bands*effective_number_k_points_list-1,number_g_points_list-1);
		return rho_reduced;
	}
};
void Dipole_Elements::print(vec excitonic_momentum, vec q_parameter, int which_term, int diagonal_k){
	cx_mat exponential_factor=function_building_exponential_factor(excitonic_momentum);
	
	//for(int g=0;g<number_g_points_list;g++)
	//	cout<<exponential_factor.col(g)<<" ";
	///for(int i=0;i<number_k_points_list;i++)
	///	cout<<k_points_list.col(i)<<" ";
	
	tuple<cx_mat,cx_mat> energies_and_dipole_elements=pull_values(excitonic_momentum,excitonic_momentum,q_parameter,diagonal_k);
	cx_mat energies; energies=get<0>(energies_and_dipole_elements);
	cx_mat dipole_elements;	dipole_elements=get<1>(energies_and_dipole_elements);

	int effective_number_k_points_list;
	if(diagonal_k==0)
		effective_number_k_points_list=number_k_points_list*number_k_points_list;
	else
		effective_number_k_points_list=number_k_points_list;

	int states1;
	if(which_term==0){
		states1=spin_number_valence_plus_conduction*number_valence_plus_conduction*effective_number_k_points_list;
	}else if(which_term==1){
		dipole_elements=pull_reduced_values_vc(dipole_elements,diagonal_k);
		states1=spin_number_valence_bands*number_conduction_bands*effective_number_k_points_list;
	}else if(which_term==2){
		dipole_elements=pull_reduced_values_cc_vv(dipole_elements,0,diagonal_k);
		states1=spin_number_conduction_bands*number_conduction_bands*effective_number_k_points_list;
	}else if(which_term==4){
		dipole_elements=pull_reduced_values_cv(dipole_elements,diagonal_k);
		states1=spin_number_valence_bands*number_conduction_bands*effective_number_k_points_list;
	}else{
		dipole_elements=pull_reduced_values_cc_vv(dipole_elements,1,diagonal_k);
		states1=spin_number_valence_bands*number_valence_bands*effective_number_k_points_list;
	}
	for(int i=0;i<states1;i++){
		for(int g=0;g<number_g_points_list;g++)
			cout<<dipole_elements(i,g)<<" ";
		cout<<endl;
	}
};
/// Dielectric_Function
class Dielectric_Function
{
private:
	int number_k_points_list;
	int number_g_points_list;
	mat g_points_list;
	int htb_basis_dimension;
	int number_valence_bands;
	int number_conduction_bands;
	vec excitonic_momentum;
	Dipole_Elements *dipole_elements;
	Coulomb_Potential *coulomb_potential;
	int spinorial_calculation;
	cx_mat rho_reduced;
	mat energies;
public:
	Dielectric_Function(Dipole_Elements *dipole_elements_tmp,int number_k_points_list_tmp,int number_g_points_list_tmp,mat g_points_list_tmp,int number_valence_bands_tmp,int number_conduction_bands_tmp,Coulomb_Potential *coulomb_potential_tmp,int spinorial_calculation_tmp);
	cx_mat pull_values(vec excitonic_momentum,cx_double omega,double eta);
	cx_mat pull_values_PPA(vec excitonic_momentum,cx_double omega,double eta,double PPA);
	void print(vec excitonic_momentum,cx_double omega,double eta,double PPA,int which_term);
	void pull_macroscopic_value(cx_vec omegas_path,int number_omegas_path,double eta,string file_macroscopic_dielectric_function_name);
	~Dielectric_Function(){
		coulomb_potential=NULL;
		dipole_elements=NULL;
	};
};
Dielectric_Function::Dielectric_Function(Dipole_Elements *dipole_elements_tmp,int number_k_points_list_tmp,int number_g_points_list_tmp,mat g_points_list_tmp,int number_valence_bands_tmp,int number_conduction_bands_tmp,Coulomb_Potential *coulomb_potential_tmp,int spinorial_calculation_tmp){
	number_conduction_bands=number_conduction_bands_tmp;
	number_valence_bands=number_valence_bands_tmp;
	number_k_points_list=number_k_points_list_tmp;
	number_g_points_list=number_g_points_list_tmp;
	dipole_elements=dipole_elements_tmp;
	coulomb_potential=coulomb_potential_tmp;
	g_points_list=g_points_list_tmp;
	spinorial_calculation=spinorial_calculation_tmp;
};
cx_mat Dielectric_Function::pull_values(vec excitonic_momentum, cx_double omega, double eta){
	cx_mat epsiloninv; epsiloninv.zeros(number_g_points_list,number_g_points_list);
	cx_double ieta; ieta.real(0.0); ieta.imag(eta);

	vec zeros_vec(3,fill::zeros);
	tuple<cx_mat,cx_mat> energies_rho=dipole_elements->pull_values(excitonic_momentum,excitonic_momentum,zeros_vec,1);
	cx_mat rho_cv=dipole_elements->pull_reduced_values_cv(get<1>(energies_rho),1); cx_mat energies=get<0>(energies_rho);
	cx_vec coulomb_shifted(number_g_points_list);
	cx_double ione; ione.real(1.0); ione.imag(0.0);
	
	//auto t1 = std::chrono::high_resolution_clock::now();
	/// defining the denominator factors
	cx_mat rho_reduced_single_column_modified((spinorial_calculation+1)*number_k_points_list*number_conduction_bands*number_valence_bands,number_g_points_list);
	cx_vec multiplicative_factor((spinorial_calculation+1)*number_k_points_list*number_conduction_bands*number_valence_bands);
	for(int spin_channel=0;spin_channel<(spinorial_calculation+1);spin_channel++){
		#pragma omp parallel for collapse(3) 
		for(int i=0;i<number_k_points_list;i++)
			for(int c=0;c<number_conduction_bands;c++)
				for(int v=0;v<number_valence_bands;v++){
					multiplicative_factor(spin_channel*number_conduction_bands*number_valence_bands*number_k_points_list+c*number_valence_bands*number_k_points_list+v*number_k_points_list+i)= ione / (omega + energies(spin_channel,c*number_valence_bands*number_k_points_list+v*number_k_points_list+i) + ieta) -  ione / (omega - energies(spin_channel,c*number_valence_bands*number_k_points_list+v*number_k_points_list+i) - ieta);
					///cout<<"multi "<<multiplicative_factor(spin_channel*number_conduction_bands*number_valence_bands*number_k_points_list+c*number_valence_bands*number_k_points_list+v*number_k_points_list+i)<<endl;
				}
	}
	
	#pragma omp parallel for
	for(int i=0;i<number_g_points_list;i++){
		coulomb_shifted(i).real(coulomb_potential->pull(excitonic_momentum+g_points_list.col(i)));
		rho_reduced_single_column_modified.col(i)=rho_cv.col(i)%multiplicative_factor;
		///cout<<"rho "<<rho_cv.col(i)<<endl;
		///cout<<"rho m "<<rho_reduced_single_column_modified.col(i)<<endl;
	}
	#pragma omp parallel for collapse(2)
	for(int i=0;i<number_g_points_list;i++)
		for(int j=0;j<number_g_points_list;j++){
			epsiloninv(i,j)=coulomb_shifted(i)*accu(conj(rho_cv.col(i))%rho_reduced_single_column_modified.col(j));
			///cout<<"epsil "<<epsiloninv(i,j)<<endl;
		}
	#pragma omp parallel for 
	for(int i=0;i<number_g_points_list;i++)
		epsiloninv(i,i).real(epsiloninv(i,i).real()+1.0);
	///auto t2 = std::chrono::high_resolution_clock::now();
	///cout<< std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count();
    ///cout<< " milliseconds\n";
	return epsiloninv;
};
cx_mat Dielectric_Function::pull_values_PPA(vec excitonic_momentum,cx_double omega,double eta,double PPA){
	cx_double omega_PPA; omega_PPA.imag(PPA); omega_PPA.real(0.0);
	cx_double omega_0; omega_0.real(0.0); omega_0.imag(0.0);
	cx_double ieta; ieta.real(0.0); ieta.imag(eta);
	
	cx_mat epsiloninv_0=pull_values(excitonic_momentum,omega_0,eta);
	cx_mat epsiloninv_PPA=pull_values(excitonic_momentum,omega_PPA,eta);
	
	cx_mat rgg(number_g_points_list,number_g_points_list);
	cx_mat ogg(number_g_points_list,number_g_points_list);
	
	ogg=PPA*sqrt(epsiloninv_PPA/(epsiloninv_0-epsiloninv_PPA));
	rgg=(epsiloninv_0%ogg)/2;
	
	cx_mat epsilon_app(number_g_points_list,number_g_points_list,fill::zeros);

	for(int i=0;i<number_g_points_list;i++)
		for(int j=0;j<number_g_points_list;j++)
			epsilon_app(i,j)=rgg(i,j)*(1.0/(omega-ogg(i,j)+ieta)-1.0/(omega+ogg(i,j)-ieta));
	for(int i=0;i<number_g_points_list;i++)
		epsilon_app(i,i)+=1.0;
	return epsilon_app;
};
void Dielectric_Function::pull_macroscopic_value(cx_vec omegas_path,int number_omegas_path,double eta,string file_macroscopic_dielectric_function_name){
	cx_mat macroscopic_dielectric_function(number_g_points_list,number_g_points_list);
	vec q_point_0(3,fill::zeros);
	cx_double ieta; ieta.real(0.0); ieta.imag(eta);
	q_point_0(0)+=minval;
	int g_point_0=int(number_g_points_list/2);
	ofstream file_macroscopic_dielectric_function;
	file_macroscopic_dielectric_function.open(file_macroscopic_dielectric_function_name);
	for(int i=0;i<number_omegas_path;i++){
		///cout<<i<<" "<<number_omegas_path<<g_points_list.col(g_point_0)<<" "<<pull_values(q_point_0,omegas_path(i),eta)(g_point_0,g_point_0)<<endl;
		///cout<<i<<" "<<pull_values(q_point_0,omegas_path(i),eta)<<endl;
		macroscopic_dielectric_function=(pull_values(q_point_0,omegas_path(i),eta)).i();
		file_macroscopic_dielectric_function<<i<<" "<<omegas_path(i)<<" "<<macroscopic_dielectric_function(g_point_0,g_point_0)<<endl;
	}
	file_macroscopic_dielectric_function.close();
};
void Dielectric_Function::print(vec excitonic_momentum,cx_double omega,double eta,double PPA,int which_term){
	cx_mat dielectric_function(number_g_points_list,number_g_points_list);
	if(which_term==0)
		dielectric_function=pull_values(excitonic_momentum,omega,eta);
	else
		dielectric_function=pull_values_PPA(excitonic_momentum,omega,eta,PPA);
	
	for(int i=0;i<number_g_points_list;i++){
		for(int j=0;j<number_g_points_list;j++)
			cout<<dielectric_function(i,j)<<" ";	
		cout<<endl;
	}
};	
/// Excitonic_Hamiltonian class
class Excitonic_Hamiltonian
{
private:
	int spinorial_calculation;
	int number_valence_bands;
	int number_conduction_bands;
	int number_valence_plus_conduction;
	int dimension_bse_hamiltonian;
	int spin_dimension_bse_hamiltonian;
	int spin_number_valence_plus_conduction;
	int htb_basis_dimension;
	int number_k_points_list;
	int number_valence_times_conduction;
	int number_g_points_list;
	mat k_points_list;
	mat g_points_list;
	mat exciton;
	mat exciton_spin;
	Coulomb_Potential *coulomb_potential;
	Dielectric_Function *dielectric_function;
	Hamiltonian_TB *hamiltonian_tb;
	Dipole_Elements *dipole_elements;
	int adding_screening;
	double volume_cell;
public:
	/// be carefull: do not try to build the BSE matrix with more bands than those given by the hamiltonian!!!
	/// there is a check at the TB hamiltonian level but not here...
	Excitonic_Hamiltonian(int number_valence_bands_tmp,int number_conduction_bands_tmp,Coulomb_Potential *coulomb_potential_tmp,Dielectric_Function *dielectric_function_tmp, Hamiltonian_TB *hamiltonian_tb_tmp, Dipole_Elements *dipole_elements_tmp, mat k_points_list_tmp, int number_k_points_list_tmp, mat g_points_list_tmp,int number_g_points_list_tmp, int spinorial_calculation_tmp, int adding_screening_tmp);
	tuple<cx_mat,cx_mat> pull_excitonic_hamiltonian_and_rcv(vec excitonic_momentum,double eta,int separated,int tamn_dancoff);
	tuple<cx_vec,cx_mat,cx_mat> common_diagonalization(vec excitonic_momentum,double eta,int separated,int tamn_dancoff);
	tuple<vec,cx_mat,cx_mat> cholesky_diagonalization(vec excitonic_momentum,double eta,int tamn_dancoff);
	cx_vec pull_excitonic_oscillator_force(cx_mat excitonic_eigenstates,cx_mat rho_cv);
	void pull_macroscopic_bse_dielectric_function(cx_vec omegas_path,int number_omegas_path,double eta,string file_macroscopic_dielectric_function_bse_name,double lorentzian,int tamn_dancoff);
	void print(vec excitonic_momentum,double eta,int tamn_dancoff);
};
Excitonic_Hamiltonian::Excitonic_Hamiltonian(int number_valence_bands_tmp,int number_conduction_bands_tmp,Coulomb_Potential *coulomb_potential_tmp,Dielectric_Function *dielectric_function_tmp,Hamiltonian_TB *hamiltonian_tb_tmp,Dipole_Elements *dipole_elements_tmp,mat k_points_list_tmp,int number_k_points_list_tmp,mat g_points_list_tmp,int number_g_points_list_tmp,int spinorial_calculation_tmp,int adding_screening_tmp){
	spinorial_calculation = spinorial_calculation_tmp;
	number_k_points_list = number_k_points_list_tmp;
	number_conduction_bands = number_conduction_bands_tmp;
	number_valence_bands = number_valence_bands_tmp;
	number_valence_times_conduction = number_conduction_bands*number_valence_bands;
	number_valence_plus_conduction = number_conduction_bands + number_valence_bands;
	dimension_bse_hamiltonian = number_k_points_list * number_conduction_bands * number_valence_bands;
	
	k_points_list = k_points_list_tmp;
	g_points_list = g_points_list_tmp;
	number_g_points_list = number_g_points_list_tmp;
	hamiltonian_tb = hamiltonian_tb_tmp;
	coulomb_potential = coulomb_potential_tmp;
	htb_basis_dimension = hamiltonian_tb->pull_htb_basis_dimension();
	volume_cell=coulomb_potential->pull_volume();
	
	adding_screening=adding_screening_tmp;
	dielectric_function=dielectric_function_tmp;
	dipole_elements=dipole_elements_tmp;

	exciton.set_size(2, number_valence_bands*number_conduction_bands);
	int e = 0;
	for (int v = 0; v < number_valence_bands; v++)
		for (int c = 0; c < number_conduction_bands; c++){
			exciton(0, e) = c;
			exciton(1, e) = v;
			e++;
		}
	/// defining the possible spin combinations
	exciton_spin.zeros(2, 4);
	exciton_spin(1, 1) = 1;
	exciton_spin(0, 2) = 1;
	exciton_spin(0, 3) = 1;
	exciton_spin(1, 3) = 1;
	
	if (spinorial_calculation == 1){
		spin_dimension_bse_hamiltonian = dimension_bse_hamiltonian * 4;
		spin_number_valence_plus_conduction = number_valence_plus_conduction * 2;
	}else{
		spin_dimension_bse_hamiltonian = dimension_bse_hamiltonian;
		spin_number_valence_plus_conduction = number_valence_plus_conduction;
	}
};

tuple<cx_mat,cx_mat> Excitonic_Hamiltonian::pull_excitonic_hamiltonian_and_rcv(vec excitonic_momentum,double eta,int separated,int tamn_dancoff){
	
	///calculating screening
	cx_mat epsilon_inv_static(number_g_points_list,number_g_points_list);
	if(adding_screening==1){
		cx_double omega_0; omega_0.real(0.0); omega_0.imag(0.0);
		epsilon_inv_static=dielectric_function->pull_values(excitonic_momentum,omega_0,eta);
	}else{
		epsilon_inv_static.eye();
	}
	/// calculating the potentianl before the BSE hamiltonian building
	/// calculating the generalized potential (the screened one and the unscreened-one)
	cx_mat v_coulomb_qg(number_g_points_list,number_g_points_list);
	cx_vec v_coulomb_g(number_g_points_list);
	for (int k = 0; k < number_g_points_list; k++){
		for (int s = 0; s < number_g_points_list; s++)
			v_coulomb_qg(k,s) = epsilon_inv_static(k,s)*coulomb_potential->pull(-(excitonic_momentum+g_points_list.col(s)));
		v_coulomb_g(k) = coulomb_potential->pull(g_points_list.col(k));
	}

	///building entry 00 (resonant part)
	cout<<"building 00"<<endl;
	///building v
	cout<<"building v"<<endl;
	vec zeros_vec(3,fill::zeros);
	vec minus_excitonic_momentum(3,fill::zeros);
	for(int i=0;i<3;i++)
		minus_excitonic_momentum(i)=-1.0*excitonic_momentum(i);
	tuple<cx_mat,cx_mat> energies_rho_q_diagk=dipole_elements->pull_values(excitonic_momentum,zeros_vec,excitonic_momentum,1);
	cx_mat rho_q_diagk_cv=dipole_elements->pull_reduced_values_cv(get<1>(energies_rho_q_diagk),1);
	cx_mat energies_q=get<0>(energies_rho_q_diagk);
	cx_mat v_matrix(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian,fill::zeros);
	cx_mat temporary_matrix1((spinorial_calculation+1)*number_conduction_bands*number_valence_bands*number_k_points_list,number_g_points_list);
	for(int i=0;i<(spinorial_calculation+1)*number_conduction_bands*number_valence_bands*number_k_points_list;i++)
		temporary_matrix1.row(i)=rho_q_diagk_cv.row(i)%v_coulomb_g.t();
	for(int i=0;i<(spinorial_calculation+1);i++)
		for(int j=0;j<(spinorial_calculation+1);j++)
			v_matrix.submat(i*3*number_conduction_bands*number_valence_bands*number_k_points_list,j*3*number_conduction_bands*number_valence_bands*number_k_points_list,(i*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1,(j*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1)=
				conj(rho_q_diagk_cv.submat(i*number_conduction_bands*number_valence_bands*number_k_points_list,0,(1+i)*number_conduction_bands*number_valence_bands*number_k_points_list-1,number_g_points_list-1))
				*(temporary_matrix1.submat(j*number_conduction_bands*number_valence_bands*number_k_points_list,0,(j+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1,number_g_points_list-1)).t();
	
	(get<0>(energies_rho_q_diagk)).reset();
	(get<1>(energies_rho_q_diagk)).reset();

	///building w
	cout<<"building w"<<endl;
	cx_mat w_matrix(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian,fill::zeros);
	tuple<cx_mat,cx_mat> energies_rho_kk=dipole_elements->pull_values(excitonic_momentum,zeros_vec,zeros_vec,0);
	cx_mat rho_kk_cc=dipole_elements->pull_reduced_values_cc_vv(get<1>(energies_rho_kk),0,0);
	tuple<cx_mat,cx_mat> energies_rho_qq_kk=dipole_elements->pull_values(excitonic_momentum,excitonic_momentum,excitonic_momentum,0);
	cx_mat rho_qq_kk_vv=dipole_elements->pull_reduced_values_cc_vv(get<1>(energies_rho_qq_kk),1,0);
	(get<0>(energies_rho_kk)).reset();
	(get<1>(energies_rho_kk)).reset();
	(get<0>(energies_rho_qq_kk)).reset();
	(get<1>(energies_rho_qq_kk)).reset();

	cx_mat temporary_matrix2((spinorial_calculation+1)*number_conduction_bands*number_conduction_bands*number_k_points_list*number_k_points_list,number_g_points_list);
	temporary_matrix2=conj(rho_kk_cc)*v_coulomb_qg;
	cx_mat temporary_matrix3((spinorial_calculation+1)*number_valence_bands*number_valence_bands*number_k_points_list*number_k_points_list,(spinorial_calculation+1)*number_conduction_bands*number_conduction_bands*number_k_points_list*number_k_points_list);
	temporary_matrix3=(temporary_matrix2)*(rho_qq_kk_vv.t());
	temporary_matrix2.reset();
	///ordering the w matrix
	field<umat> col_indices(spinorial_calculation+1);
	field<umat> row_indices(spinorial_calculation+1);
	for(int spin=0;spin<(spinorial_calculation+1);spin++){
		col_indices(spin).set_size(number_valence_bands*number_k_points_list,number_valence_bands*number_k_points_list);
		row_indices(spin).set_size(number_conduction_bands*number_k_points_list,number_conduction_bands*number_k_points_list);
	}
	for(int spinv1=0;spinv1<(spinorial_calculation+1);spinv1++){
		#pragma omp parallel for collapse(2) 
		for(int k1=0;k1<number_k_points_list;k1++)
			for(int k2=0;k2<number_k_points_list;k2++){
				for(int v1=0;v1<number_valence_bands;v1++)
					for(int v2=0;v2<number_valence_bands;v2++)
						col_indices(spinv1)(v1*number_k_points_list+k1,v2*number_k_points_list+k2)=spinv1*number_valence_bands*number_valence_bands*number_k_points_list*number_k_points_list+v1*number_valence_bands*number_k_points_list*number_k_points_list+v2*number_k_points_list*number_k_points_list+k1*number_k_points_list+k2;
				for(int c1=0;c1<number_conduction_bands;c1++)
					for(int c2=0;c2<number_conduction_bands;c2++)
						row_indices(spinv1)(c1*number_k_points_list+k1,c2*number_k_points_list+k2)=spinv1*number_conduction_bands*number_conduction_bands*number_k_points_list*number_k_points_list+c1*number_conduction_bands*number_k_points_list*number_k_points_list+c2*number_k_points_list*number_k_points_list+k1*number_k_points_list+k2;
			}
	}
	int spinv1; int spinc1; 
	cx_mat temporary_matrix4(number_conduction_bands*number_k_points_list,number_valence_bands*number_k_points_list);
	cx_vec temporary_matrix5(number_conduction_bands*number_valence_bands*number_k_points_list);
	for(int spin1=0;spin1<(3*spinorial_calculation+1);spin1++){
		spinv1=exciton_spin(0,spin1);
		spinc1=exciton_spin(1,spin1);
		for(int c1=0;c1<number_conduction_bands;c1++)
			for(int v1=0;v1<number_valence_bands;v1++)
				for(int k1=0;k1<number_k_points_list;k1++){
					temporary_matrix4=temporary_matrix3(row_indices(spinc1).col(c1*number_k_points_list+k1),col_indices(spinv1).col(v1*number_k_points_list+k1));
					for(int c2=0;c2<number_conduction_bands;c2++)
						for(int v2=0;v2<number_conduction_bands;v2++)
							temporary_matrix5.subvec(c2*number_valence_bands*number_k_points_list+v2*number_k_points_list,c2*number_valence_bands*number_k_points_list+(v2+1)*number_k_points_list-1)=diagvec(temporary_matrix4.submat(c2*number_k_points_list,v2*number_k_points_list,(c2+1)*number_k_points_list-1,(v2+1)*number_k_points_list-1));
					w_matrix.submat(spin1*number_conduction_bands*number_valence_bands*number_k_points_list+c1*number_valence_bands*number_k_points_list+v1*number_k_points_list+k1,
						spin1*number_conduction_bands*number_valence_bands*number_k_points_list,spin1*number_conduction_bands*number_valence_bands*number_k_points_list+c1*number_valence_bands*number_k_points_list+v1*number_k_points_list+k1,
						(spin1+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1)=temporary_matrix5.t();
				}
	}
	temporary_matrix3.reset();
	temporary_matrix4.reset();
	temporary_matrix5.reset();

	cx_mat w_matrix_c(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian,fill::zeros);
	cx_mat v_matrix_c(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian,fill::zeros);
	if(tamn_dancoff==0){
		///building entry 01 (coupling part)
		cout<<"building 01"<<endl;
		///building v
		cout<<"building v"<<endl;
		tuple<cx_mat,cx_mat> energies_rho_p_diagk=dipole_elements->pull_values(excitonic_momentum,minus_excitonic_momentum,zeros_vec,1);
		cx_mat rho_p_diagk_vc=dipole_elements->pull_reduced_values_vc(get<1>(energies_rho_p_diagk),1);
		for(int i=0;i<(spinorial_calculation+1);i++)
			for(int j=0;j<(spinorial_calculation+1);j++)
				v_matrix_c.submat(i*3*number_conduction_bands*number_valence_bands*number_k_points_list,j*3*number_conduction_bands*number_valence_bands*number_k_points_list,(i*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1,(j*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1)=
					conj(rho_p_diagk_vc.submat(i*number_conduction_bands*number_valence_bands*number_k_points_list,0,(1+i)*number_conduction_bands*number_valence_bands*number_k_points_list-1,number_g_points_list-1))
					*(temporary_matrix1.submat(j*number_conduction_bands*number_valence_bands*number_k_points_list,0,(j+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1,number_g_points_list-1)).t();

		temporary_matrix1.reset();
		rho_p_diagk_vc.reset();
		(get<0>(energies_rho_p_diagk)).reset();
		(get<1>(energies_rho_p_diagk)).reset();

		///building W
		cout<<"building w"<<endl;
		tuple<cx_mat,cx_mat> energies_rho_p_kk=dipole_elements->pull_values(excitonic_momentum,minus_excitonic_momentum,zeros_vec,0);
		cx_mat rho_p_kk_vc=dipole_elements->pull_reduced_values_vc(get<1>(energies_rho_p_kk),0);
		tuple<cx_mat,cx_mat> energies_rho_q_kk=dipole_elements->pull_values(excitonic_momentum,zeros_vec,excitonic_momentum,0);
		cx_mat rho_q_kk_cv=dipole_elements->pull_reduced_values_cv(get<1>(energies_rho_q_kk),0);
		(get<0>(energies_rho_p_kk)).reset();
		(get<1>(energies_rho_p_kk)).reset();
		(get<0>(energies_rho_q_kk)).reset();
		(get<1>(energies_rho_q_kk)).reset();

		cx_mat temporary_matrix6((spinorial_calculation+1)*number_conduction_bands*number_valence_bands*number_k_points_list*number_k_points_list,number_g_points_list);
		temporary_matrix6=conj(rho_q_kk_cv)*v_coulomb_qg;
		rho_q_kk_cv.reset();
		cx_mat temporary_matrix7((spinorial_calculation+1)*number_conduction_bands*number_valence_bands*number_k_points_list*number_k_points_list,(spinorial_calculation+1)*number_valence_bands*number_conduction_bands*number_k_points_list*number_k_points_list);
		temporary_matrix7=temporary_matrix6*(rho_p_kk_vc.t());
		rho_p_kk_vc.reset();

		///ordering the w matrix
		cout<<"ordering w"<<endl;
		field<umat> row_indices_c(spinorial_calculation+1);

		for(int spinc1=0;spinc1<(spinorial_calculation+1);spinc1++)
			row_indices_c(spinc1).set_size(number_valence_bands*number_k_points_list,number_conduction_bands*number_k_points_list);

		for(int spinc1=0;spinc1<(spinorial_calculation+1);spinc1++)
			for(int k1=0;k1<number_k_points_list;k1++)
				for(int k2=0;k2<number_k_points_list;k2++)
					for(int c1=0;c1<number_conduction_bands;c1++)
						for(int v2=0;v2<number_valence_bands;v2++)
							row_indices_c(spinc1)(v2*number_k_points_list+k2,c1*number_k_points_list+k1)=spinc1*number_conduction_bands*number_valence_bands*number_k_points_list*number_k_points_list+c1*number_conduction_bands*number_valence_bands*number_k_points_list+v2*number_k_points_list*number_k_points_list+k1*number_k_points_list+k2;

		cx_mat temporary_matrix8(number_valence_bands*number_k_points_list,number_conduction_bands*number_k_points_list);
		cx_vec temporary_matrix9(number_conduction_bands*number_k_points_list*number_valence_bands);

		int spin2;
		for(int spin1=0;spin1<(3*spinorial_calculation+1);spin1++){
			spinv1=exciton_spin(0,spin1);
			spinc1=exciton_spin(1,spin1);
			if((spin1==0)||(spin1==3*spinorial_calculation+1)){
				spin2=spin1;
			}else
				spin2=(2-spin1)+1;
			for(int c1=0;c1<number_conduction_bands;c1++)
				for(int v1=0;v1<number_valence_bands;v1++)
					for(int k1=0;k1<number_k_points_list;k1++){
						temporary_matrix8=temporary_matrix7.submat(row_indices_c(spinc1).col(c1*number_k_points_list+k1),row_indices_c(spinv1).row(v1*number_k_points_list+k1));
						for(int c2=0;c2<number_conduction_bands;c2++)
							for(int v2=0;v2<number_conduction_bands;v2++)
								temporary_matrix9.subvec(c2*number_valence_bands*number_k_points_list+v2*number_k_points_list,c2*number_valence_bands*number_k_points_list+(v2+1)*number_k_points_list-1)=(temporary_matrix8.submat(c2*number_k_points_list,v2*number_k_points_list,(c2+1)*number_k_points_list-1,(v2+1)*number_k_points_list-1)).diag();
						w_matrix_c.submat(spin1*number_conduction_bands*number_valence_bands*number_k_points_list+c1*number_valence_bands*number_k_points_list+v1*number_k_points_list+k1,
							spin2*number_conduction_bands*number_valence_bands*number_k_points_list,spin1*number_conduction_bands*number_valence_bands*number_k_points_list+c1*number_valence_bands*number_k_points_list+v1*number_k_points_list+k1,
							(spin2+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1)=temporary_matrix9.t();
					}
		}

		temporary_matrix6.reset();
		temporary_matrix7.reset();
		temporary_matrix8.reset();
		temporary_matrix9.reset();
	}

	///separated == 1 gives H00=Resonant Part H11=Coupling Part
	///rewriting energies in order to obtain something that can be summed to the rest
	cx_vec energies_q_vc((spinorial_calculation+1)*number_conduction_bands*number_valence_bands*number_k_points_list);
	for(int spin=0;spin<(spinorial_calculation+1);spin++)
		for(int c=0;c<number_conduction_bands;c++)
			for(int v=0;v<number_valence_bands;v++){
				energies_q_vc.subvec(spin*number_conduction_bands*number_valence_bands*number_k_points_list+c*number_valence_bands*number_k_points_list+v*number_k_points_list,spin*number_conduction_bands*number_valence_bands*number_k_points_list+c*number_valence_bands*number_k_points_list+(v+1)*number_k_points_list-1)=energies_q.submat(spin,v*number_conduction_bands*number_k_points_list+c*number_k_points_list,spin,v*number_conduction_bands*number_k_points_list+(c+1)*number_k_points_list-1).t();
				//cout<<energies_0_cv.subvec(spin*number_conduction_bands*number_valence_bands*number_k_points_list+c*number_valence_bands*number_k_points_list+v*number_k_points_list,spin*number_conduction_bands*number_valence_bands*number_k_points_list+c*number_valence_bands*number_k_points_list+(v+1)*number_k_points_list-1)<<endl;
			}
	/// saving memory for the BSE matrix (kernel)
	cx_mat excitonic_hamiltonian(2*spin_dimension_bse_hamiltonian,2*spin_dimension_bse_hamiltonian);
	///cout<<"volume"<<volume_cell<<endl;
	w_matrix=w_matrix/(number_k_points_list);
	w_matrix_c=w_matrix_c/(number_k_points_list);
	v_matrix=v_matrix/(number_k_points_list);
	v_matrix_c=v_matrix/(number_k_points_list);
	if (separated==0){
		///building excitonic hamiltonian in the case of q-->0 (in other cases it is necessary to build the other entries correctly)
		excitonic_hamiltonian.submat(0,0,spin_dimension_bse_hamiltonian-1,spin_dimension_bse_hamiltonian-1)=((2-spinorial_calculation)*v_matrix-w_matrix);
		excitonic_hamiltonian.submat(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian,2*spin_dimension_bse_hamiltonian-1,2*spin_dimension_bse_hamiltonian-1)=-(conj((2-spinorial_calculation)*v_matrix-w_matrix));
		for(int i=0;i<(spinorial_calculation+1);i++){
			excitonic_hamiltonian.submat(i*3*number_conduction_bands*number_valence_bands*number_k_points_list,i*3*number_conduction_bands*number_valence_bands*number_k_points_list,(i*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1,(i*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1)+=
				diagmat(-energies_q_vc.subvec(i*number_conduction_bands*number_valence_bands*number_k_points_list,(i+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1));
			excitonic_hamiltonian.submat(spin_dimension_bse_hamiltonian+i*3*number_conduction_bands*number_valence_bands*number_k_points_list,spin_dimension_bse_hamiltonian+i*3*number_conduction_bands*number_valence_bands*number_k_points_list,spin_dimension_bse_hamiltonian+(i*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1,spin_dimension_bse_hamiltonian+(i*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1)+=
				diagmat(conj(energies_q_vc.subvec(i*number_conduction_bands*number_valence_bands*number_k_points_list,(i+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1)));
		}
		excitonic_hamiltonian.submat(0,spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian-1,2*spin_dimension_bse_hamiltonian-1)=((2-spinorial_calculation)*v_matrix_c-w_matrix_c);
		excitonic_hamiltonian.submat(spin_dimension_bse_hamiltonian,0,2*spin_dimension_bse_hamiltonian-1,spin_dimension_bse_hamiltonian-1)=-((conj((2-spinorial_calculation)*v_matrix_c-w_matrix_c))).t();
		return {excitonic_hamiltonian,rho_q_diagk_cv};
	}else{
		excitonic_hamiltonian.submat(0,0,spin_dimension_bse_hamiltonian-1,spin_dimension_bse_hamiltonian-1)=v_matrix-w_matrix;
		for(int i=0;i<2;i++)
			excitonic_hamiltonian.submat(i*3*number_conduction_bands*number_valence_bands*number_k_points_list,i*3*number_conduction_bands*number_valence_bands*number_k_points_list,(i*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1,(i*3+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1)+=
				+diagmat(-energies_q_vc.subvec(i*number_conduction_bands*number_valence_bands*number_k_points_list,(i+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1));
		excitonic_hamiltonian.submat(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian,2*spin_dimension_bse_hamiltonian-1,2*spin_dimension_bse_hamiltonian-1)=v_matrix_c-w_matrix_c;
		return {excitonic_hamiltonian,rho_q_diagk_cv};
	}
};
/// usual diagonalization routine
tuple<cx_vec,cx_mat,cx_mat> Excitonic_Hamiltonian::common_diagonalization(vec excitonic_momentum,double eta,int separated,int tamn_dancoff)
{
	tuple<cx_mat,cx_mat> excitonic_hamiltonian_and_rho_cv=pull_excitonic_hamiltonian_and_rcv(excitonic_momentum,eta,0,tamn_dancoff);
	cx_mat excitonic_hamiltonian=get<0>(excitonic_hamiltonian_and_rho_cv);
	cx_mat rho_cv=get<1>(excitonic_hamiltonian_and_rho_cv);

	
	/// diagonalizing the BSE matrix
	/// M_{(bz_number_k_points_list x number_valence_bands x number_conduction_bands)x(bz_number_k_points_list x number_valence_bands x number_conduction_bands)}
	if (spinorial_calculation == 1){
		int dimension_bse_hamiltonian_2=2*spin_dimension_bse_hamiltonian;
		
		
		///SISTEMARE DIMENSIONI
		///SISTEMARE SOTTOMATRICI CONSIDERATE, TENERE CONTO THE COUPLING PARTE E RESONANT PARTE SONO STATE COSTRUITE SEPARATAMENTE

		///separating the excitonic hamiltonian in two blocks; still not the ones associated to the magnons and excitons
		cx_mat excitonic_hamiltonian_0(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian);
		cx_mat excitonic_hamiltonian_1(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian);
		excitonic_hamiltonian_1=excitonic_hamiltonian.submat(number_conduction_bands*number_valence_bands*number_k_points_list,number_conduction_bands*number_valence_bands*number_k_points_list,3*number_conduction_bands*number_valence_bands*number_k_points_list-1,3*number_conduction_bands*number_valence_bands*number_k_points_list-1);
		///cout<<excitonic_hamiltonian_1_tmp.n_cols<<" "<<excitonic_hamiltonian_1_tmp.n_rows<<endl;
		for(int i=0;i<2;i++){
			excitonic_hamiltonian_0.submat(i*dimension_bse_hamiltonian,i*dimension_bse_hamiltonian,(i+1)*dimension_bse_hamiltonian-1,(i+1)*dimension_bse_hamiltonian-1)=
				excitonic_hamiltonian.submat(3*i*number_conduction_bands*number_valence_bands*number_k_points_list,3*i*number_conduction_bands*number_valence_bands*number_k_points_list,(3*i+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1,(3*i+1)*number_conduction_bands*number_valence_bands*number_k_points_list-1);
		}

		///diagonalizing the two spin channels separately: M=0 and M=\pm1
		cx_vec eigenvalues_1(dimension_bse_hamiltonian_2); 
		cx_mat eigenvectors_1(dimension_bse_hamiltonian_2,dimension_bse_hamiltonian_2);
		cx_vec eigenvalues_0(dimension_bse_hamiltonian_2); 
		cx_mat eigenvectors_0(dimension_bse_hamiltonian_2,dimension_bse_hamiltonian_2);
		lapack_complex_double *temporary_0;
		temporary_0=(lapack_complex_double*)malloc(dimension_bse_hamiltonian_2*dimension_bse_hamiltonian_2*sizeof(lapack_complex_double)); 
		lapack_complex_double *temporary_1;
		temporary_1=(lapack_complex_double*)malloc(dimension_bse_hamiltonian_2*dimension_bse_hamiltonian_2*sizeof(lapack_complex_double)); 

		#pragma omp parallel for collapse(2)
		for(int i=0;i<dimension_bse_hamiltonian_2;i++)
			for(int j=0;j<dimension_bse_hamiltonian_2;j++){
				temporary_0[i*dimension_bse_hamiltonian_2+j]=real(excitonic_hamiltonian_0(i,j))+_Complex_I*imag(excitonic_hamiltonian_0(i,j));
				temporary_1[i*dimension_bse_hamiltonian_2+j]=real(excitonic_hamiltonian_1(i,j))+_Complex_I*imag(excitonic_hamiltonian_1(i,j));
			}
	
		int N=dimension_bse_hamiltonian_2;
		int LDA=dimension_bse_hamiltonian_2;
		int LDVL=1;
		int LDVR=dimension_bse_hamiltonian_2;
		char JOBVR='V';
		char JOBVL='N';
		int matrix_layout = 101;
		int INFO0; int INFO1;
		lapack_complex_double *empty;

		lapack_complex_double *w_0;
		w_0=(lapack_complex_double*)malloc(N*sizeof(lapack_complex_double));
		lapack_complex_double *u_0;
		u_0=(lapack_complex_double*)malloc(N*LDVR*sizeof(lapack_complex_double));
		lapack_complex_double *w_1;
		w_1=(lapack_complex_double*)malloc(N*sizeof(lapack_complex_double));
		lapack_complex_double *u_1;
		u_1=(lapack_complex_double*)malloc(N*LDVR*sizeof(lapack_complex_double));
		
		INFO0 = LAPACKE_zgeev(matrix_layout,JOBVL,JOBVR,N,temporary_0,LDA,w_0,empty,LDVL,u_0,LDVR);
		INFO1 = LAPACKE_zgeev(matrix_layout,JOBVL,JOBVR,N,temporary_1,LDA,w_1,empty,LDVL,u_1,LDVR);
		
		free(temporary_0); free(temporary_1);
		//eig_gen(eigenvalues_1,eigenvectors_1,excitonic_hamiltonian_1);
		//eig_gen(eigenvalues_0,eigenvectors_0,excitonic_hamiltonian_0);
		for(int i=0;i<dimension_bse_hamiltonian_2;i++){
			eigenvalues_0(i).real(lapack_complex_double_real(w_0[i]));
			eigenvalues_0(i).imag(lapack_complex_double_imag(w_0[i]));
			eigenvalues_1(i).real(lapack_complex_double_real(w_1[i]));
			eigenvalues_1(i).imag(lapack_complex_double_imag(w_1[i]));
		}
		///ordering the eigenvalues and saving them in a single matrix exc_eigenvalues
		cx_vec exc_eigenvalues(spin_dimension_bse_hamiltonian); uvec ordering_0=sort_index(eigenvalues_0);
		cx_mat exc_eigenvectors(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian);
		/// normalizing and ordering eigenvectors: saving them in a single matrix exc_eigenvectors
		for(int i=0;i<dimension_bse_hamiltonian_2;i++){
			for(int s=0;s<dimension_bse_hamiltonian_2;s++)
				exc_eigenvectors(s,i)=u_0[s*dimension_bse_hamiltonian_2+ordering_0(i)]; 
			exc_eigenvalues(i) = eigenvalues_0(ordering_0(i));
		}

		/// separating magnons and excitons
		///to add routine separating the two parts
		uvec ordering_1=sort_index(eigenvalues_1.subvec(0,dimension_bse_hamiltonian));
		for(int i=0;i<dimension_bse_hamiltonian;i++){
			for(int s=0;s<dimension_bse_hamiltonian;s++)
				exc_eigenvectors(s+dimension_bse_hamiltonian_2,i+dimension_bse_hamiltonian_2)=u_1[s*dimension_bse_hamiltonian+ordering_1(i)]; 
			exc_eigenvalues(i+dimension_bse_hamiltonian_2) = eigenvalues_1(ordering_1(i));
		}
		for(int i=0;i<dimension_bse_hamiltonian;i++){
			for(int s=0;s<dimension_bse_hamiltonian;s++)
			exc_eigenvectors(s+dimension_bse_hamiltonian_2+dimension_bse_hamiltonian,i+dimension_bse_hamiltonian_2+dimension_bse_hamiltonian)=u_1[(s+1)*dimension_bse_hamiltonian+i+dimension_bse_hamiltonian]; 
			exc_eigenvalues(i+dimension_bse_hamiltonian_2+dimension_bse_hamiltonian) = eigenvalues_1(i+dimension_bse_hamiltonian);
		}
		for(int i=0;i<spin_dimension_bse_hamiltonian;i++)
			exc_eigenvectors.col(i)=exc_eigenvectors.col(i)/norm(exc_eigenvectors.col(i),2);

		free(w_0); free(w_1); free(u_0); free(u_1);
		return {exc_eigenvalues,exc_eigenvectors,rho_cv};
	}else{
		int dimension_bse_hamiltonian_2=2*dimension_bse_hamiltonian;
		cx_vec eigenvalues(dimension_bse_hamiltonian_2); 
		cx_mat eigenvectors(dimension_bse_hamiltonian_2,dimension_bse_hamiltonian_2);
		
		lapack_complex_double *temporary;
		temporary=(lapack_complex_double*)malloc(dimension_bse_hamiltonian_2*dimension_bse_hamiltonian_2*sizeof(lapack_complex_double));
		
		#pragma omp parallel for collapse(2)
		for(int i=0;i<dimension_bse_hamiltonian_2;i++)
			for(int j=0;j<dimension_bse_hamiltonian_2;j++)
				temporary[i*dimension_bse_hamiltonian_2+j]=real(excitonic_hamiltonian(i,j))+_Complex_I*imag(excitonic_hamiltonian(i,j));	
				
		int N=dimension_bse_hamiltonian_2;
		int LDA=dimension_bse_hamiltonian_2;
		int LDVL=dimension_bse_hamiltonian_2;
		int LDVR=dimension_bse_hamiltonian_2;
		char JOBVR='N';
		char JOBVL='V';
		int matrix_layout = 101;
		int INFO;
		lapack_complex_double *w;
		w=(lapack_complex_double*)malloc(N*sizeof(lapack_complex_double));
		lapack_complex_double *u;
		u=(lapack_complex_double*)malloc(N*LDVR*sizeof(lapack_complex_double));
		lapack_complex_double *empty;
		empty=(lapack_complex_double*)malloc(N*LDVL*sizeof(lapack_complex_double));

		///WORK=(lapack_complex_double*)malloc(LWORK*sizeof(lapack_complex_double));
		//RWORK=(double*)malloc(LWORK*sizeof(double));
		INFO = LAPACKE_zgeev(matrix_layout,JOBVL,JOBVR,N,temporary,LDA,w,u,LDVR,empty,LDVL);
		cout<<INFO<<endl;
		free(empty); 
		///cout<<u<<endl;
		for(int i=0;i<dimension_bse_hamiltonian_2;i++){
			eigenvalues(i).real(lapack_complex_double_real(w[i]));
			eigenvalues(i).imag(lapack_complex_double_imag(w[i]));
		}
		///ordering the eigenvalues and saving them in a single matrix exc_eigenvalues
		cx_vec exc_eigenvalues(dimension_bse_hamiltonian_2); 
		uvec ordering=sort_index(real(eigenvalues)); 
		cx_mat exc_eigenvectors(dimension_bse_hamiltonian_2,dimension_bse_hamiltonian_2);
		
		/// normalizing and ordering eigenvectors: saving them in a single matrix exc_eigenvectors
		for(int i=0;i<dimension_bse_hamiltonian_2;i++){
			for(int s=0;s<dimension_bse_hamiltonian_2;s++){
				exc_eigenvectors(s,i).real(lapack_complex_double_real(u[s*dimension_bse_hamiltonian_2+ordering(i)]));
				exc_eigenvectors(s,i).imag(lapack_complex_double_imag(u[s*dimension_bse_hamiltonian_2+ordering(i)]));
			}
			exc_eigenvalues(i) = eigenvalues(ordering(i));
			//if(i>=dimension_bse_hamiltonian)
			///	cout<<"eig "<<exc_eigenvalues(i)<<endl;
		}

		for(int i=0;i<dimension_bse_hamiltonian_2;i++)
			exc_eigenvectors.col(i)=exc_eigenvectors.col(i)/norm(exc_eigenvectors.col(i),2);
		free(u);
		return {exc_eigenvalues.subvec(dimension_bse_hamiltonian,2*dimension_bse_hamiltonian-1),exc_eigenvectors.submat(dimension_bse_hamiltonian,dimension_bse_hamiltonian,2*dimension_bse_hamiltonian-1,2*dimension_bse_hamiltonian-1),rho_cv};
		///return{ exc_eigenvalues,exc_eigenvectors,rho_cv};
	}
};

/// Fastest diagonalization routine
///[1] Structure preserving parallel algorithms for solving the Bethe–Salpeter eigenvalue problem Meiyue Shao, Felipe H. da Jornada, Chao Yang, Jack Deslippe, Steven G. Louie
///[2] Beyond the Tamm-Dancoff approximation for ext.ended systems using exact diagonalization Tobias Sander, Emanuelel Maggio, and Georg Kresse
tuple<vec,cx_mat,cx_mat> Excitonic_Hamiltonian:: cholesky_diagonalization(vec excitonic_momentum, double eta,int tamn_dancoff)
{
	tuple<cx_mat,cx_mat> resonant_part_and_coupling_part_and_rhocv=pull_excitonic_hamiltonian_and_rcv(excitonic_momentum,eta,1,tamn_dancoff);
	cx_mat rho_cv=get<1>(resonant_part_and_coupling_part_and_rhocv);
	/// diagonalizing the BSE matrix M_{(bz_number_k_points_list x number_valence_bands x number_conduction_bands)x(bz_number_k_points_list x number_valence_bands x number_conduction_bands)}
	int spin_dimension_bse_hamiltonian_2 = 2*spin_dimension_bse_hamiltonian;
	cx_mat T=get<0>(resonant_part_and_coupling_part_and_rhocv);
	cx_mat A=T.submat(0,0,spin_dimension_bse_hamiltonian-1,spin_dimension_bse_hamiltonian-1);
	cx_mat B=T.submat(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian,2*spin_dimension_bse_hamiltonian-1,2*spin_dimension_bse_hamiltonian-1);

	mat M(spin_dimension_bse_hamiltonian_2, spin_dimension_bse_hamiltonian_2);
	// Fill top-left submatrix
	M.submat(0, 0, spin_dimension_bse_hamiltonian - 1, spin_dimension_bse_hamiltonian - 1) = real(A + B);
	// Fill bottom-left submatrix (as conjugate transpose of top-right submatrix)
	M.submat(spin_dimension_bse_hamiltonian, 0, spin_dimension_bse_hamiltonian_2 - 1, spin_dimension_bse_hamiltonian - 1) = -imag(A + B).t();
	// Fill top-right submatrix (as conjugate transpose of bottom-left submatrix)
	M.submat(0, spin_dimension_bse_hamiltonian, spin_dimension_bse_hamiltonian - 1, spin_dimension_bse_hamiltonian_2 - 1) = -imag(A-B);
	// Fill bottom-right submatrix
	M.submat(spin_dimension_bse_hamiltonian, spin_dimension_bse_hamiltonian, spin_dimension_bse_hamiltonian_2 - 1, spin_dimension_bse_hamiltonian_2 - 1) = real(A - B);
	
	///symmetrizing
	M+=M.t();
	M=M/2;
	cout<<M.is_symmetric()<<endl;

	cout<<"cholesky factorization"<<endl;
	/// construct W
	vec diag_one(spin_dimension_bse_hamiltonian,fill::ones);
	mat J(spin_dimension_bse_hamiltonian_2,spin_dimension_bse_hamiltonian_2,fill::zeros);
	J.submat(0,spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian-1,spin_dimension_bse_hamiltonian_2-1)=diagmat(diag_one);
	J.submat(spin_dimension_bse_hamiltonian,0,spin_dimension_bse_hamiltonian_2-1,spin_dimension_bse_hamiltonian-1)=-diagmat(diag_one);
	
	mat L(spin_dimension_bse_hamiltonian_2, spin_dimension_bse_hamiltonian_2,fill::zeros);
	//L=chol(M);
	
	int N=spin_dimension_bse_hamiltonian_2;
	int LDA=spin_dimension_bse_hamiltonian_2;
	int matrix_layout = 101;
	int INFO;
	char UPLO = 'U';
	double *temporary_0; temporary_0 = (double *)malloc(spin_dimension_bse_hamiltonian_2*spin_dimension_bse_hamiltonian_2*sizeof(double));
	#pragma omp parallel for collapse(2)
	for(int i=0;i<spin_dimension_bse_hamiltonian_2;i++)
		for(int j=0;j<spin_dimension_bse_hamiltonian_2;j++){
			temporary_0[i*spin_dimension_bse_hamiltonian+j]=M(i,j);
		}

	INFO=LAPACKE_dpotrf(matrix_layout, UPLO, N, temporary_0, LDA);

	for (int i = 0; i < spin_dimension_bse_hamiltonian_2; i++)
		for (int j = i; j < spin_dimension_bse_hamiltonian_2; j++){
			L(i,j)=temporary_0[i * spin_dimension_bse_hamiltonian_2 + j];
		}

	free(temporary_0);

	mat W(spin_dimension_bse_hamiltonian_2,spin_dimension_bse_hamiltonian_2);
	W =  L.t() * J;
	W = W * L;
	
	lapack_complex_float *temporary_1; temporary_1 = (lapack_complex_float *)malloc(spin_dimension_bse_hamiltonian_2*spin_dimension_bse_hamiltonian_2*sizeof(lapack_complex_float));
	
	#pragma omp parallel for collapse(2)
	for(int i=0;i<spin_dimension_bse_hamiltonian_2;i++)
		for(int j=0;j<spin_dimension_bse_hamiltonian_2;j++){
			temporary_1[i*spin_dimension_bse_hamiltonian_2+j]=imag(W(i,j))-_Complex_I*real(W(i,j));
			//cout<<temporary_1[i*spin_dimension_bse_hamiltonian_2+j]<<endl;
			cout<<W(i,j)<<" ";
		}

	float *w;
	char JOBZ = 'V';
	char JOBU = 'A';
	char JOBVT = 'A';
	int F=spin_dimension_bse_hamiltonian_2;
	int LDU=spin_dimension_bse_hamiltonian_2;
	int LDVT=spin_dimension_bse_hamiltonian_2;
	lapack_complex_float *z1;
	lapack_complex_float *z2;
	z1=(lapack_complex_float*)malloc(F*F*sizeof(lapack_complex_float));
	z2=(lapack_complex_float*)malloc(F*F*sizeof(lapack_complex_float));
	//// saving all the eigenvalues
	w = (float *)malloc(spin_dimension_bse_hamiltonian_2 * sizeof(float));
	float *lwork; lwork=(float*)malloc(4*F*sizeof(float));

	cout<<"singular value decomposition"<<endl;
	INFO=LAPACKE_cgesvd(matrix_layout,JOBU,JOBVT,F,N,temporary_1,LDA,w,z1,LDU,z2,LDVT,lwork);
	
	free(temporary_1);

	vec exc_eigenvalues(spin_dimension_bse_hamiltonian);
	for(int i=0;i<spin_dimension_bse_hamiltonian;i++){
		exc_eigenvalues(i)=-w[i];
		cout<<exc_eigenvalues(i)<<endl;
	}
	free(w);

	cx_mat exc_eigenvectors(spin_dimension_bse_hamiltonian_2,spin_dimension_bse_hamiltonian_2);
	cx_mat z_(spin_dimension_bse_hamiltonian_2,spin_dimension_bse_hamiltonian_2);
	for(int i=0;i<spin_dimension_bse_hamiltonian_2;i++)
		for(int j=0;j<spin_dimension_bse_hamiltonian_2;j++)
			z_(i,j)=z1[i*spin_dimension_bse_hamiltonian_2+j];
	free(z1); free(z2);
		
	cx_double ieta;
	ieta.real(0.0); ieta.imag(eta);

	exc_eigenvectors.set_real(J*L);
	exc_eigenvectors=exc_eigenvectors*z_;

	exc_eigenvectors.submat(0,0,spin_dimension_bse_hamiltonian-1,spin_dimension_bse_hamiltonian-1)=exc_eigenvectors.submat(0,0,spin_dimension_bse_hamiltonian-1,spin_dimension_bse_hamiltonian-1)*inv(diagmat(sqrt(exc_eigenvalues)+ieta));
	exc_eigenvectors.submat(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian_2-1,spin_dimension_bse_hamiltonian_2-1)=exc_eigenvectors.submat(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian_2-1,spin_dimension_bse_hamiltonian_2-1)*inv(diagmat(sqrt(exc_eigenvalues)+ieta));

	return{exc_eigenvalues,exc_eigenvectors.submat(0,0,spin_dimension_bse_hamiltonian-1,spin_dimension_bse_hamiltonian-1),rho_cv};
};

cx_vec Excitonic_Hamiltonian::pull_excitonic_oscillator_force(cx_mat excitonic_eigenstates,cx_mat rho_cv){
	
	cx_vec oscillator_force;
	int number_g_0=int(number_g_points_list/2);

	if(spinorial_calculation==1){
		cx_mat excitonic_eigenstates_reduced=excitonic_eigenstates.submat(0,0,2*dimension_bse_hamiltonian-1,2*dimension_bse_hamiltonian-1);	
		oscillator_force.zeros(dimension_bse_hamiltonian);
		for(int i=0;i<dimension_bse_hamiltonian;i++){
			//excitonic_eigenstates_reduced.col(i)=excitonic_eigenstates_reduced.col(i)%multiplicative_factor;
			oscillator_force(i)=accu(rho_cv.col(number_g_0)%conj(excitonic_eigenstates_reduced.col(i)));
		}
	}else{
		oscillator_force.zeros(dimension_bse_hamiltonian);
		///cout<<"ecco"<<g_points_list.col(number_g_0)<<endl;
		for(int i=0;i<dimension_bse_hamiltonian;i++){
			//excitonic_eigenstates.col(i)=excitonic_eigenstates.col(i)%multiplicative_factor;
			oscillator_force(i)=accu(conj(rho_cv.col(number_g_0))%excitonic_eigenstates.col(i));
			//cout<<excitonic_eigenstates.col(i)<<endl;
		}
	}
	return oscillator_force;
};
void Excitonic_Hamiltonian:: pull_macroscopic_bse_dielectric_function(cx_vec omegas_path,int number_omegas_path,double eta,string file_macroscopic_dielectric_function_bse_name,double lorentzian,int tamn_dancoff)
{
	//cout << "Calculating dielectric tensor..." << endl;
	
	double factor = 8*pigreco/((minval*minval)*coulomb_potential->pull_volume());
	//cx_mat rho_dipoles=htb->pull_dipoles(excitonic_momentum,number_valence_bands,number_conduction_bands,eta);

	tuple<cx_vec,cx_mat,cx_mat> eigenvalues_and_eigenstates_and_rho_cv;
	cx_mat rho_cv(spin_number_valence_plus_conduction*number_valence_plus_conduction*number_k_points_list,number_g_points_list);
	
	cx_vec exc_eigenvalues_tmp1(spin_dimension_bse_hamiltonian);
	cx_vec exc_eigenvalues_tmp2(spin_dimension_bse_hamiltonian);
	cx_vec exc_eigenvalues(spin_dimension_bse_hamiltonian);
	
	//vec exc_eigenvalues_tmp1(spin_dimension_bse_hamiltonian);
	//vec exc_eigenvalues_tmp2(spin_dimension_bse_hamiltonian);
	//vec exc_eigenvalues(spin_dimension_bse_hamiltonian);
	
	cx_double exc_energy;

	cx_mat exc_eigenstates(spin_dimension_bse_hamiltonian,spin_dimension_bse_hamiltonian);
	mat energies_q(2,number_valence_times_conduction*number_k_points_list);

	cx_cube dielectric_tensor_bse(3,3,number_omegas_path,fill::zeros);
	
	cx_double ieta;	ieta.real(0.0);	ieta.imag(eta);
	cx_double ilorentzian; ilorentzian.real(0.0); ilorentzian.imag(lorentzian);
	cx_vec temporary_variable(number_omegas_path);
	
	//cout<<"dimension: "<<size(dielectric_tensor_bse)<<endl;
	vec excitonic_momentum1(3); vec excitonic_momentum2(3);
	cx_vec exc_oscillator_force1(dimension_bse_hamiltonian, fill::zeros);
	cx_vec exc_oscillator_force2(dimension_bse_hamiltonian, fill::zeros);
	cx_mat delta(3,3,fill::zeros);
	for(int i=0;i<3;i++)
		delta(i,i).real(1.0);

	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++){
			if(i==j&&i==0){
			//	excitonic_momentum1(i)+=minval;
			//	excitonic_momentum2(j)+=minval;
			//	exc_eigenvalues.zeros(spin_dimension_bse_hamiltonian);
			//	eigenvalues_and_eigenstates_and_rho_cv = pull_eigenstates_through_cholesky_way(excitonic_momentum1, epsilon, eta);
			//	exc_eigenvalues_tmp1=get<0>(eigenvalues_and_eigenstates_and_rho_cv);
			//	exc_eigenstates=get<1>(eigenvalues_and_eigenstates_and_rho_cv);
			//	rho_cv=get<2>(eigenvalues_and_eigenstates_and_rho_cv);
			//	exc_oscillator_force1=pull_excitonic_oscillator_force(exc_eigenstates,rho_cv);
			//	eigenvalues_and_eigenstates_and_rho_cv = pull_eigenstates_through_cholesky_way(excitonic_momentum2, epsilon, eta);
			//	exc_eigenvalues_tmp2=get<0>(eigenvalues_and_eigenstates_and_rho_cv);
			//	exc_eigenstates=get<1>(eigenvalues_and_eigenstates_and_rho_cv);
			//	rho_cv=get<2>(eigenvalues_and_eigenstates_and_rho_cv);
			//	exc_oscillator_force2=pull_excitonic_oscillator_force(exc_eigenstates,rho_cv);
			//	for(int r=0;r<spin_dimension_bse_hamiltonian;r++)
			//		exc_eigenvalues(r)=(exc_eigenvalues_tmp1(r)+exc_eigenvalues_tmp2(r))/2.0;
			//	excitonic_momentum1(i)-=minval;
			//	excitonic_momentum2(j)-=minval;
			//}else{
			excitonic_momentum1(i)+=minval;
			eigenvalues_and_eigenstates_and_rho_cv = common_diagonalization(excitonic_momentum1,eta,0,tamn_dancoff);
			//eigenvalues_and_eigenstates_and_rho_cv_and_energies_q = pull_eigenstates_through_cholesky_way(excitonic_momentum1, eta);
			exc_eigenvalues=get<0>(eigenvalues_and_eigenstates_and_rho_cv);
			exc_eigenstates=get<1>(eigenvalues_and_eigenstates_and_rho_cv);
			rho_cv=get<2>(eigenvalues_and_eigenstates_and_rho_cv);
			exc_oscillator_force1=pull_excitonic_oscillator_force(exc_eigenstates,rho_cv);
			for(int l=0;l<dimension_bse_hamiltonian;l++)
				exc_oscillator_force2(l)=exc_oscillator_force1(l);
			excitonic_momentum1(i)-=minval;
			///}
			//cout<<"dimension ex: "<<size(exc_oscillator_force1)<<endl;
			for(int s=0;s<number_omegas_path;s++)
				temporary_variable(s)=0.0;
			/////#pragma omp parallel for private(exc_oscillator_force2,exc_oscillator_force1,omega,exc_eigenvalues,dielectric_tensor_bse)
			for(int s=0;s<number_omegas_path;s++){
				///cout<<s<<" "<<number_omegas_path<<endl;
				for(int l=0;l<dimension_bse_hamiltonian;l++){
					temporary_variable(s)+=exc_oscillator_force1(l)*conj(exc_oscillator_force1(l))/(omegas_path(s)-exc_eigenvalues(l)+ilorentzian);
				}
				dielectric_tensor_bse(i,j,s)=delta(i,j)-factor*temporary_variable(s);
			}
			}
		}

	ofstream dielectric_tensor_file;
	dielectric_tensor_file.open(file_macroscopic_dielectric_function_bse_name);
	///writing the dielectric function (in the optical limit) in a file
	dielectric_tensor_file<<"### omega xx xy xz yx yy yz zx zy zz"<<endl;
	for(int s=0;s<number_omegas_path;s++){
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				dielectric_tensor_file<<omegas_path(s)<<" "<<dielectric_tensor_bse(i,j,s)<<" ";
		dielectric_tensor_file<<endl;
	}
	dielectric_tensor_file.close();
};
void Excitonic_Hamiltonian::print(vec excitonic_momentum,double eta,int tamn_dancoff){
	cout<<"BSE hamiltoian..."<<endl;
	tuple<cx_mat,cx_mat> hamiltonian_and_rho=pull_excitonic_hamiltonian_and_rcv(excitonic_momentum,eta,0,tamn_dancoff);
	cx_mat hamiltonian=get<0>(hamiltonian_and_rho);
	for(int i=0;i<spin_dimension_bse_hamiltonian;i++){
		for(int j=0;j<spin_dimension_bse_hamiltonian;j++)
			printf("%4.10f   ",hamiltonian(i,j).real());
		cout<<endl;
	}
	
	cout<<"Eigenvalues..."<<endl;
	tuple<cx_vec,cx_mat,cx_mat> eigenvalues_and_eigenstates_and_rho_cv_and_energies_q;
	eigenvalues_and_eigenstates_and_rho_cv_and_energies_q=common_diagonalization(excitonic_momentum,eta,0,tamn_dancoff);
	cx_vec eigenvalues=get<0>(eigenvalues_and_eigenstates_and_rho_cv_and_energies_q);
	cx_mat eigenstates=get<1>(eigenvalues_and_eigenstates_and_rho_cv_and_energies_q);
	for (int i=0;i<spin_dimension_bse_hamiltonian;i++)
		cout<<eigenvalues(i)<<endl;
};

int main()
{
	double fermi_energy = 5.5427;
	////Initializing Lattice
	string file_crystal_bravais_name="bravais.lattice_si.data";
	string file_crystal_coordinates_name="atoms_si.data";
	int number_atoms=2;
	Crystal_Lattice crystal(file_crystal_bravais_name,file_crystal_coordinates_name,number_atoms);
	double volume=crystal.pull_volume();
	crystal.print();

	////Initializing k points list
	vec shift; shift.zeros(3);
	shift(0)=minval;
	shift(1)=minval;
	K_points k_points(&crystal,shift);
	string file_k_points_name="k_points_list_si.dat";
	int number_k_points_list=10;
	k_points.push_k_points_list_values(file_k_points_name,number_k_points_list);
	mat k_points_list=k_points.pull_k_points_list_values();
	k_points.print();

	//////Initializing g points list
	vec shift_g; shift_g.zeros(3);;
	double cutoff_g_points_list=2; int dimension_g_points_list=3;
	vec direction_cutting(3); direction_cutting(0)=1; direction_cutting(1)=1; direction_cutting(2)=1;
	G_points g_points(&crystal,cutoff_g_points_list,dimension_g_points_list,direction_cutting,shift_g);
	mat g_points_list=g_points.pull_g_points_list_values(); 
	int number_g_points_list=g_points.pull_number_g_points_list();
	cout<<"G points: "<<number_g_points_list<<endl;
	///g_points.print();
	
	//////Initializing Coulomb potentia
	double minimum_k_point_modulus=1.0e-12;
	int dimension_potential=3;
	Coulomb_Potential coulomb_potential(&k_points,&g_points,minimum_k_point_modulus,dimension_potential,direction_cutting,volume);
	//string file_coulomb_name="coulomb.dat"; 
	//int number_k_points_c=10000; double max_k_points_radius_c=1.0e-8;
	//int direction_profile_xyz=0;
	//coulomb_potential.print_profile(number_k_points_c,max_k_points_radius_c,file_coulomb_name,direction_profile_xyz);

	////Initializing the Tight Binding hamiltonian (saving the Wannier functions centers)
	ifstream file_htb; ifstream file_centers; string seedname;
	string wannier90_hr_file_name="tb_no_spin_polarized.dat";
	string wannier90_centers_file_name="tb_no_spin_polarized_centers.dat";
	bool dynamic_shifting=false;
	int spinorial_calculation = 0;
	double little_shift=0.00;
	double scissor_operator=0.00;
	Hamiltonian_TB htb(wannier90_hr_file_name,wannier90_centers_file_name,fermi_energy,spinorial_calculation,number_atoms,dynamic_shifting,little_shift,scissor_operator);
	/// 0 no spinors, 1 collinear spinors, 2 non-collinear spinors (implementing 0 and 1 cases)
	int number_wannier_centers=htb.pull_number_wannier_functions();
	int htb_basis_dimension=htb.pull_htb_basis_dimension();
	//cout<<number_wannier_centers<<endl;
	//htb.print();
	//vec k_point; k_point.zeros(3); k_point(0)=1.0;
	//htb.FFT(k_point);
	//htb.print_ks_states(k_point,4,4);

	//////Initializing dipole elements
	int number_conduction_bands_selected=4;
	int number_valence_bands_selected=4;
	Dipole_Elements dipole_elements(number_k_points_list,k_points_list,number_g_points_list,g_points_list,number_wannier_centers,number_valence_bands_selected,number_conduction_bands_selected,&htb,spinorial_calculation);
	vec excitonic_momentum; excitonic_momentum.zeros(3);
	excitonic_momentum(0)=minval;
	//vec vec_zero(3,fill::zeros);
	//dipole_elements.print(excitonic_momentum,excitonic_momentum,4,0);

	/////////Initializing dielectric function
	Dielectric_Function dielectric_function(&dipole_elements,number_k_points_list,number_g_points_list,g_points_list,number_valence_bands_selected,number_conduction_bands_selected,&coulomb_potential,spinorial_calculation);
	cx_double omega; omega=0.0; double eta=10e-4; 
	////double PPA=27.00;
	int number_omegas_path=400;
	cx_vec omegas_path(number_omegas_path);
	cx_double max_omega=5.00;
	cx_double min_omega=0.00;
	cx_vec macroscopic_dielectric_function(number_omegas_path);
	for(int i=0;i<number_omegas_path;i++)
		omegas_path(i)=min_omega+double(i)/double(number_omegas_path)*(max_omega-min_omega);
	//string file_macroscopic_dielectric_function_name="macroscopic_diel_func.dat";
	//dielectric_function.pull_macroscopic_value(omegas_path,number_omegas_path,eta,file_macroscopic_dielectric_function_name);

	////////Initializing BSE hamiltonian
	int adding_screening=1;
	double lorentzian=1.0e-1;
	int tamn_dancoff=0;
	Excitonic_Hamiltonian htbse(number_valence_bands_selected,number_conduction_bands_selected,&coulomb_potential,&dielectric_function,&htb,&dipole_elements,k_points_list,number_k_points_list,g_points_list,number_g_points_list,spinorial_calculation,adding_screening);
	///htbse.print(excitonic_momentum,eta,tamn_dancoff);
	///calculation optical spectrum
	string file_macroscopic_dielectric_function_bse_name="macroscopic_diel_func_bse.dat";
	htbse.pull_macroscopic_bse_dielectric_function(omegas_path,number_omegas_path,eta,file_macroscopic_dielectric_function_bse_name,lorentzian,tamn_dancoff);
	return 1;
}