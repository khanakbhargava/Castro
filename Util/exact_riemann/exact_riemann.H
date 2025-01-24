#ifndef EXACT_RIEMANN_H
#define EXACT_RIEMANN_H

#include <AMReX_REAL.H>

#include <fstream>

#include <riemann_sample.H>
#include <riemann_star_state.H>
#include <riemann_support.H>

using namespace amrex::literals;

AMREX_INLINE
void
exact_riemann() {

    // we need a composition to interface with our EOS, but we are not
    // exploring composition jumps here.  We'll take a constant
    // composition.

    amrex::Real xn[NumSpec] = {0.0};
    xn[0] = 1.0_rt;


    // if we are using T as the independent variable (rather than p), then
    // get p now
    if (problem::use_Tinit) {

        eos_t eos_state;
        eos_state.rho = problem::rho_l;
        eos_state.T = problem::T_l;
        for (int n = 0; n < NumSpec; ++n) {
            eos_state.xn[n] = xn[n];
        }

        eos(eos_input_rt, eos_state);

        problem::p_l = eos_state.p;

        eos_state.rho = problem::rho_r;
        eos_state.T = problem::T_r;

        eos(eos_input_rt, eos_state);

        problem::p_r = eos_state.p;

    }

    if (problem::co_moving_frame) {
        amrex::Real W_avg = 0.5_rt * (problem::u_l + problem::u_r);
        problem::u_l -= W_avg;
        problem::u_r -= W_avg;
    }

    amrex::Real ustar, pstar, W_l, W_r;

    riemann_star_state(problem::rho_l, problem::u_l, problem::p_l, xn,
                       problem::rho_r, problem::u_r, problem::p_r, xn,
                       ustar, pstar, W_l, W_r);


    // find the solution as a function of xi = x/t
    std::ofstream ofile;
    ofile.open("riemann.out");

    // This follows from the discussion around C&G Eq. 15

    amrex::Real dx = (problem::xmax - problem::xmin) / static_cast<amrex::Real>(problem::npts);

    // loop over xi space

    std::cout << "evaluating at t = " << problem::t << std::endl;

    for (int i = 1; i <= problem::npts; i++) {

        // compute xi = x/t -- this is the similarity variable for the
        // solution
        amrex::Real x  = problem::xmin + (static_cast<amrex::Real>(i) - 0.5_rt) * dx;

        amrex::Real rho, u, p, xn_s[NumSpec];

        riemann_sample(problem::rho_l, problem::u_l, problem::p_l, xn,
                       problem::rho_r, problem::u_r, problem::p_r, xn,
                       ustar, pstar, W_l, W_r,
                       x, problem::xjump, problem::t,
                       rho, u, p, xn_s);

        if (problem::co_moving_frame) {
            amrex::Real W_avg = 0.5_rt * (problem::u_l + problem::u_r);
            u += W_avg;
            x += problem::t * W_avg;
        }

        // get the thermodynamics for this state for output

        eos_t eos_state;
        eos_state.rho = rho;
        eos_state.p = p;
        for (int n = 0; n < NumSpec; ++n) {
            eos_state.xn[n] = xn_s[n];
        }
        eos_state.T = problem::initial_temp_guess;

        eos(eos_input_rp, eos_state);

        if (i == 1) {
            ofile << "# i ";
            ofile << std::setw(26) << "x"
                  << std::setw(26) << "rho"
                  << std::setw(26) << "u"
                  << std::setw(26) << "p"
                  << std::setw(26) << "T"
                  << std::setw(26) << "e"
                  << std::setw(26) << "gamma_1" << std::endl;
        }

        ofile << std::setw(4) << i;
        ofile << std::setw(26) << std::setprecision(12) << x
              << std::setw(26) << std::setprecision(12) << rho
              << std::setw(26) << std::setprecision(12) << u
              << std::setw(26) << std::setprecision(12) << p
              << std::setw(26) << std::setprecision(12) << eos_state.T
              << std::setw(26) << std::setprecision(12) << eos_state.e
              << std::setw(26) << std::setprecision(12) << eos_state.gam1 << std::endl;

    }

    ofile.close();
}
#endif
