/**
 * @file main.cpp
 * @brief Solver for the 2D Laplace problem with VTK output.
 */

#include <iostream>
#include <cstdio>
#include <cmath>
#include <numbers>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>


/**
 * @brief Compute the source term f(x,y).
 *
 * @param x x-coordinate in the domain.
 * @param y y-coordinate in the domain.
 * @return Value of f(x,y).
 */
double source_term(const double& x, const double& y) {
    double f;

    f = 8*std::numbers::pi*std::numbers::pi * std::sin(2*std::numbers::pi*x) * std::sin(2*std::numbers::pi*y);

    return f;
}


/**
 * @brief Compute the analytical solution u(x,y).
 *
 * @param x x-coordinate in the domain.
 * @param y y-coordinate in the domain.
 * @return Value of the exact solution u(x,y).
 */
double exact_solution(const double& x, const double& y) {
    double sol;

    sol = std::sin(2*std::numbers::pi*x) * std::sin(2*std::numbers::pi*y);

    return sol;
}


/**
 * @brief Compute the weighted L2 norm of the difference between two vectors.
 *
 * @param u First vector.
 * @param v Second vector.
 * @param w Weighting factor used in the norm.
 * @return Weighted L2 norm of u - v.
 */
double l2_norm(const std::vector<double>& u, const std::vector<double>& v, const double& w) {
    double sum = 0;

    std::size_t nNodes = u.size();
    std::vector<double> diff(nNodes, 0);

    for(size_t k=0; k<nNodes; ++k) {
        diff[k] = u[k] - v[k];
        sum += diff[k] * diff[k];
    }
    return w*std::sqrt(sum);
}


/**
 * @brief Write a scalar field to a legacy ASCII VTK file.
 *
 * @param n Number of grid points in each coordinate direction.
 * @param h Uniform grid spacing.
 * @param solution Scalar solution values in linear storage.
 * @param x0 x-coordinate of the domain origin.
 * @param y0 y-coordinate of the domain origin.
 * @param filename Output file name without extension.
 */
void vtk_output(const int& n, const double& h, const std::vector<double>& solution, const double& x0, const double& y0, const std::string& filename) {
    const std::string path = filename + ".vtk";
    std::ofstream output(path);

    if (!output) {
        std::cerr << "Unable to open " << path << " for writing\n";
        return;
    }

    output << "# vtk DataFile Version 3.0\n";
    output << "Laplace equation solver results\n";
    output << "ASCII\n";
    output << "DATASET STRUCTURED_POINTS\n";
    output << "DIMENSIONS " << n << ' ' << n << ' ' << 1 << '\n';
    output << "ORIGIN " << x0 << ' ' << y0 << ' ' << 0 << '\n';
    output << "SPACING " << h << ' ' << h << ' ' << 1 << '\n';
    output << "POINT_DATA " << (n * n) << '\n';
    output << "SCALARS u double 1\n";
    output << "LOOKUP_TABLE default\n";

    // write values with x varying fastest (i inner, j outer)
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            const int idx = n * i + j;
            output << solution[idx] << '\n';
        }
    }

    output.close();

    std::cout << "Sucessfully wrote to " << path << '\n';
}


/**
 * @brief Remove leading and trailing whitespace from a string.
 *
 * @param s Input string.
 * @return Trimmed string.
 */
static std::string trim(std::string s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    const auto last = s.find_last_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    
    return s.substr(first, last - first + 1);
}


/**
 * @brief Program entry point.
 *
 * Reads the parameter file, solves the discrete Laplace problem, and writes
 * the numerical and analytical solutions to VTK files.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument values.
 * @return Zero on success, nonzero on failure.
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input-parameter-file>\n";
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Could not open parameter file: " << argv[1] << '\n';
        return 1;
    }

    std::string case_name;
    int N = 60;
    int maxIter = 100000;
    double tol = 1e-6;
    double x_min = 0.0, x_max = 1.0, y_min = 0.0, y_max = 1.0;

    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        const auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        line = trim(line);
        if (line.empty()) continue;

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        if (key == "NAME") {
            case_name = value;
        } else if (key == "N") {
            N = std::stoi(value);
        } else if (key == "MAXITER") {
            maxIter = std::stoi(value);
        } else if (key == "TOL") {
            tol = std::stod(value);
        } else if (key == "DOMAIN") {
            std::stringstream ss(value);
            char comma;
            ss >> x_min >> comma >> x_max >> comma >> y_min >> comma >> y_max;
        }
    }

    std::cout << "Case Name: " << case_name << '\n';
    std::cout << "N = " << N << "\n";
    std::cout << "maxIter = " << maxIter << '\n';
    std::cout << "tol = " << tol << '\n';
    std::cout << "domain = " << x_min << ", " << x_max << ", " << y_min << ", " << y_max << '\n';

    // Spatial discretization
    const double h = 1.0 / (N - 1);  // Step size (uniform)

    int k = 0;  // Nodes (linear) index
    int p = 0;  // Iteration index

    double error = 1;            // Error
    double l2 = 1;               // L2-Norm

    std::vector<double> source(N * N, 0.0);  // f(x,y)
    std::vector<double> u(N*N, 0.0);         // u(x,y) stored linear with n*n Nodes
    std::vector<double> u_new(N*N, 0.0);     // Current iteration solution (temporary)
    std::vector<double> u_exact(N*N, 0.0);   // Exact solution

    // Boundaryy conditions are zero: u(x=0) = 0, u(x=1) = 0, etc. -> fullfilled by initialization

    // Start Clock
    // Time different calculations in one run to compare
    const auto start{std::chrono::steady_clock::now()};

    while (error > tol && p < maxIter)
    {
        for (int i = 1; i < N-1; ++i) {
            double x = x_min + i * h;

            for (int j = 1; j < N-1; ++j) {
                double y = y_min + j * h;

                k = N*i + j;
                source[k] = source_term(x, y);
                u_exact[k] = exact_solution(x, y);

                //u_new[k] = 1/(4*h*h) * (u[k-n] + u[k+n] + u[k-1] + u[k+1] + source[k]);
                u_new[k] = 0.25 * (u[k-N] + u[k+N] + u[k-1] + u[k+1] + h*h * source[k]);
            }
        }
        error = l2_norm(u, u_new, h);
        l2    = l2_norm(u_exact, u_new, h);

        u = u_new;
        p++;
    }

    // Stop Clock
    const auto finish{std::chrono::steady_clock::now()};
    // Calcuate time
    const std::chrono::duration<double> elapsed_seconds{finish - start};
    const auto tp_utc{std::chrono::system_clock::now()};

    std::cout << "\nConvergence Criterion met! " << "\n--------------------\n";
    std::cout << "         Step size : " << h << '\n';
    std::cout << "    Number of Nodes: " << N*N << '\n';
    std::cout << "        Iterations : " << p << '\n';
    std::cout << "Stopping condition : " << error << '\n';
    std::cout << "           L2 Norm : " << l2 << '\n';

    std::cout << "\n---------- PERFORMANCE INFORMATION ----------" << '\n';
    // Local and elapsed time
    std::cout << "Finished Computation at: "
              << std::chrono::current_zone()->to_local(tp_utc) << '\n';
    std::cout << "           Elapsed Time: " << elapsed_seconds << '\n'; // C++20's chrono::duration operator<<

    vtk_output(N, h, u, x_min, y_min, case_name);
    vtk_output(N, h, u_exact, x_min, y_min, "analytical_solution");

    // Open a persistent pipe to Gnuplot
    FILE* gnuplot1 = popen("gnuplot -persistent", "w");
    if (!gnuplot1) {
        std::cerr << "Error: Could not open pipe to Gnuplot." << std::endl;
        return 1;
    }

    // Set up visual properties
    fprintf(gnuplot1, "set title 'Numerical Solution u_h(x,y)'\n");
    fprintf(gnuplot1, "set view map\n");           // Flat 2D heatmap projection
    fprintf(gnuplot1, "set pm3d at b\n");          // Enable color mapping
    fprintf(gnuplot1, "set palette rgbformulae 33,13,10\n"); 
    fprintf(gnuplot1, "set pm3d interpolate 2,2\n"); // Smoothes the grid blocks smoothly

    // Inline data stream command
    // '-' string tells to read directly from the pipe lines
    fprintf(gnuplot1, "splot '-' with pm3d\n");


    // Evaluate source term and stream the coordinates
    for (int i = 0; i < N; ++i) {
        double x = x_min + i * h;
        
        for (int j = 0; j < N; ++j) {
            k = N*i + j;
            double y = y_min + j * h;
            
            fprintf(gnuplot1, "%f %f %f\n", x, y, u[k]);
        }
        // Segment has ended
        fprintf(gnuplot1, "\n");
    }

    // Termination character 'e'
    fprintf(gnuplot1, "e\n");

    // Flush out buffered data strings and close the channel
    fflush(gnuplot1);
    pclose(gnuplot1);


    // Open a persistent pipe to Gnuplot
    FILE* gnuplot2 = popen("gnuplot -persistent", "w");
    if (!gnuplot2) {
        std::cerr << "Error: Could not open pipe to Gnuplot." << std::endl;
        return 1;
    }

    // Set up visual properties
    fprintf(gnuplot2, "set title 'Exact Solution u(x,y)'\n");
    fprintf(gnuplot2, "set view map\n");           // Flat 2D heatmap projection
    fprintf(gnuplot2, "set pm3d at b\n");          // Enable color mapping
    fprintf(gnuplot2, "set palette rgbformulae 33,13,10\n"); 
    fprintf(gnuplot2, "set pm3d interpolate 2,2\n"); // Smoothes the grid blocks smoothly

    // Inline data stream command
    // '-' string tells to read directly from the pipe lines
    fprintf(gnuplot2, "splot '-' with pm3d\n");


    // Evaluate source term and stream the coordinates
    for (int i = 0; i < N; ++i) {
        double x = x_min + i * h;
        
        for (int j = 0; j < N; ++j) {
            k = N*i + j;
            double y = y_min + j * h;
            
            fprintf(gnuplot2, "%f %f %f\n", x, y, u_exact[k]);
        }
        // Segment has ended
        fprintf(gnuplot2, "\n");
    }

    // Termination character 'e'
    fprintf(gnuplot2, "e\n");

    // Flush out buffered data strings and close the channel
    fflush(gnuplot2);
    pclose(gnuplot2);

    return 0;
}