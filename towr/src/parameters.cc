/******************************************************************************
Copyright (c) 2018, Alexander W. Winkler. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include <towr/parameters.h>

#include <algorithm>
#include <numeric>      // std::accumulate
#include <math.h>       // fabs
#include <cassert>

namespace towr {

Parameters::Parameters ()
{
  duration_base_polynomial_ = 0.1;

  // 2 also works quite well. Remember that in between the nodes, forces
  // could still be violating unilateral and friction constraints by
  // polynomial interpolation
  force_polynomials_per_stance_phase_ = 3;
  ee_polynomials_per_swing_phase_ = 2; // should be 2 when using swing constraint.


  dt_constraint_range_of_motion_ = 0.05;
  dt_constraint_dynamic_ = 0.1;
  dt_constraint_base_motion_ = duration_base_polynomial_/4.;

  min_phase_duration_ = 0.1;
  max_phase_duration_ = 1.0;

  force_limit_in_norm_ = 1000; // [N] this affects convergence when optimizing gait


  constraints_ = {
      BaseAcc,  // enforces that acceleration doesn't jump between splines
      EndeffectorRom,
      Dynamic,
      Terrain,
      Force,
//      TotalTime, // causes gait to also be optimized
      Swing, // remove this at some point, not so general
//      BaseRom, //  CAREFUL: restricts 6D base to be in a specific range->very limiting
  };

  // additional restrictions are set directly on the variable in nlp_factory,
  // such as e.g. initial and goal state,...

  costs_ = {
//    {ForcesCostID, 1.0},
  };
}

bool
Parameters::OptimizeTimings () const
{
  ConstraintName c = TotalTime;
  auto v = constraints_; // shorthand
  return std::find(v.begin(), v.end(), c) != v.end();
}

Parameters::VecTimes
Parameters::GetBasePolyDurations () const
{
  std::vector<double> base_spline_timings_;
  double dt = duration_base_polynomial_;
  double t_left = GetTotalTime ();

  double eps = 1e-10; // since repeated subtraction causes inaccuracies
  while (t_left > eps) {
    double duration = t_left>dt?  dt : t_left;
    base_spline_timings_.push_back(duration);

    t_left -= dt;
  }

  return base_spline_timings_;
}

int
Parameters::GetPhaseCount(EEID ee) const
{
  return ee_phase_durations_.at(ee).size();
}

int
Parameters::GetEECount() const
{
  return ee_in_contact_at_start_.size();
}

double
Parameters::GetTotalTime () const
{
  std::vector<double> T_feet;

  for (const auto& v : ee_phase_durations_)
    T_feet.push_back(std::accumulate(v.begin(), v.end(), 0.0));

  double T = T_feet.front(); // take first foot as reference

  // safety check that all feet durations sum to same value
  for (double Tf : T_feet)
    assert(fabs(Tf - T) < 1e-6);

  return T;
}

} // namespace towr
