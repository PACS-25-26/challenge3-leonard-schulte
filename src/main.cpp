#include <iostream>
#include <cstdio>
#include <cmath>
#include <numbers>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <chrono>


double source_term(const double& x, const double& y) {
    double f;

    f = 8*std::numbers::pi*std::numbers::pi * std::sin(2*std::numbers::pi*x) * std::sin(2*std::numbers::pi*y);

    return f;
}


double exact_solution(const double& x, const double& y) {
    double sol;

    sol = std::sin(2*std::numbers::pi*x) * std::sin(2*std::numbers::pi*y);

    return sol;
}


double l2_norm(const std::vector<double>& u, const std::vector<double>& v, const double& w) {
    double sum = 0;

    std::size_t nNodes = u.size();
    std::vector<double> diff(nNodes, 0);

    for(int k=0; k<nNodes; ++k) {
        diff[k] = u[k] - v[k];
        sum += diff[k] * diff[k];
    }
    return w*std::sqrt(sum);
}


void vtk_output(const int& n, const double& h, const std::vector<double>& solution, const double& x0, const double& y0, const std::string& filename) {
    // ensure output directory exists
    std::filesystem::create_directories("output");

    const std::string path = std::string("output/") + filename + ".vtk";
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
}


int main() {
    // Spatial discretization
    const int n = 60;                   // Mesh size
    const double x_min = 0, x_max = 1;  // Domain in x-direction
    const double y_min = 0, y_max = 1;  // Domain in y-direction
    const double h = 1.0 / (n - 1);     // Step size (uniform)

    int k = 0;  // Nodes (linear)
    int p = 0;  // Iteration

    const int maxIter = 100000;  // Maximum iteration
    const double tol = 1e-6;     // Convergence criterion
    double error = 1;            // Error
    double l2 = 1;               // L2-Norm

    std::vector<double> source(n * n, 0.0);  // f(x,y)
    std::vector<double> u(n*n, 0.0);         // u(x,y) stored linear with n*n Nodes
    std::vector<double> u_new(n*n, 0.0);     // Current iteration solution (temporary)
    std::vector<double> u_exact(n*n, 0.0);   // Exact solution

    // Boundaryy conditions are zero: u(x=0) = 0, u(x=1) = 0, etc. -> fullfilled by initialization

    // Start Clock
    // Time different calculations in one run to compare
    const auto start{std::chrono::steady_clock::now()};

    while (error > tol && p < maxIter)
    {
        for (int i = 1; i < n-1; ++i) {
            double x = x_min + i * h;

            for (int j = 1; j < n-1; ++j) {
                double y = y_min + j * h;

                k = n*i + j;
                source[k] = source_term(x, y);
                u_exact[k] = exact_solution(x, y);

                //u_new[k] = 1/(4*h*h) * (u[k-n] + u[k+n] + u[k-1] + u[k+1] + source[k]);
                u_new[k] = 0.25 * (u[k-n] + u[k+n] + u[k-1] + u[k+1] + h*h * source[k]);
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
    std::cout << "    Number of Nodes: " << n*n << '\n';
    std::cout << "        Iterations : " << p << '\n';
    std::cout << "Stopping condition : " << error << '\n';
    std::cout << "           L2 Norm : " << l2 << '\n';

    std::cout << "\n---------- PERFORMANCE INFORMATION ----------" << '\n';
    // Local and elapsed time
    std::cout << "Finished Computation at: "
              << std::chrono::current_zone()->to_local(tp_utc) << '\n';
    std::cout << "           Elapsed Time: " << elapsed_seconds << '\n'; // C++20's chrono::duration operator<<
    
    // Open a persistent pipe to Gnuplot
    FILE* gnuplot1 = popen("gnuplot -persistent", "w");
    if (!gnuplot1) {
        std::cerr << "Error: Could not open pipe to Gnuplot." << std::endl;
        return 1;
    }

    vtk_output(n, h, u, x_min, y_min, "numerical_solution");
    vtk_output(n, h, u_exact, x_min, y_min, "exact_solution");

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
    for (int i = 0; i < n; ++i) {
        double x = x_min + i * h;
        
        for (int j = 0; j < n; ++j) {
            k = n*i + j;
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
    for (int i = 0; i < n; ++i) {
        double x = x_min + i * h;
        
        for (int j = 0; j < n; ++j) {
            k = n*i + j;
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