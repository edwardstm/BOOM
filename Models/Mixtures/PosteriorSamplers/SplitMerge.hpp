// Copyright 2018 Google LLC. All Rights Reserved.
/*
  Copyright (C) 2005-2017 Steven L. Scott

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef BOOM_DIRICHLET_PROCESS_SPLIT_MERGE_PROPOSALS_HPP_
#define BOOM_DIRICHLET_PROCESS_SPLIT_MERGE_PROPOSALS_HPP_

#include <Models/Mixtures/DirichletProcessMixture.hpp>
#include <distributions/rng.hpp>

namespace BOOM {
  namespace SplitMerge {
    // This file contains code to implement the split-merge Metropolis-Hastings
    // move in DirichletProcessSliceSampler.  Two data points are chosen at
    // random to seed the move.  If they fall into the same mixture component
    // then a split of that component is proposed.  If they fall in different
    // components then an attempt is made to merge those components.
    //
    // The logic for how splits and merges are proposed is contained in a
    // ProposalStrategy object.

    // A Proposal is an object to be generated by a ProposalStrategy, evaluated
    // by the DirichletProcessSliceSampler, and potentially accepted by a
    // DirichletProcessMixtureModel.  A proposal encodes the information needed
    // for a DirichletProcessMixtureModel to split a component into two, or
    // merge two components into one.  It also carries information about the
    // likelihood of the split or merge being proposed, in the form of a
    // proposal density ratio needed by the Metropolis Hastings algorithm in
    // which the Proposal participates.
    //
    // When a component is split, the mathematical understanding of what happens
    // is that the first empty component is moved to a random location, new
    // parameters for the component are generated, some of the data from the
    // component to be split moves to the empty component, and the mixing
    // weights for the original and empty component are reapportioned.  When two
    // components are merged this process happens in reverse.  All data from
    // one component is moved to the other, the now-empty component is moved to
    // the first empty spot, new parameters are assigned, and mixing weights are
    // reapportioned.
    class Proposal {
     public:
      enum ProposalType {Split, Merge};

      // A Proposal has several sub-components.  Rather than have a giant
      // constructor with a dozen arguments, the constructor is kept small.
      // Users need to call set_components(), set_mixing_weights() and
      // set_log_proposal_density_ratio() before doing anything with a Proposal
      // object.  It is good practice to call check() before construction is
      // complete, to ensure that none of these steps has been forgotten.
      //
      // Args:
      //   type:  What type of proposal is this, a split or a merge?
      //   data_index_1: The index (in the primary DirichletProcessMixtureModel)
      //     of the first data point used to seed the split-merge move.
      //   data_index_2: The index (in the primary DirichletProcessMixtureModel)
      //     of the second data point used to seed the split-merge move.
      Proposal(ProposalType type, int data_index_1, int data_index_2);

      // Set the mixture components for the proposal.  Please see the
      // class-level comments about the mechanics of how the split and merge
      // moves work.  They are a transformation between pairs of components
      // (split1, split2) <--> (merged, empty).  In a merge move, 'merged' and
      // 'empty' define the state of the model before the proposed merge, while
      // 'split1' and 'split2' define the state afterwards.  In a split move the
      // opposite is true.  In either case 'merged' and 'split1' represent the
      // same component (before and after the move), as do 'split2' and 'empty'.
      //
      // Args:
      //   * merged: In a merge move, this is the proposed state of the
      //       component containing all the data from 'split2' is moved into
      //       'split1'.  In a split move it is the component to be split.
      //   * empty: The partner of 'merged'.  In a merge move this is the empty
      //       component left over after the data from split2 is moved into
      //       split1.  In a split move it is the originally empty component
      //       into which some data from 'merge' will be moved.
      //   * split1: In a merge move, this is the component which will receive
      //       the data from 'split2'.  In a split move this holds the state of
      //       'merged' after some of the data has been moved out.
      //   * split2: In a merge move, this is the component which sends all its
      //       data to 'merged / split1'.  In a split move this holds the
      //       state of 'merged' after some of the data has been moved out.
      //
      // When this function is called, the data, parameters, and mixture
      // component indices for each component should be set.  The mixture
      // component indices refer to the location of each component either before
      // or after the move (as appropriate, depending on the type of proposal).
      // This means that the mixture component indices for 'merged' and 'split1'
      // could be the same (if split2 comes after split1) or off by 1 (if split2
      // comes before split1, so that moving it to the empty position shifts
      // split1 by one spot).
      void set_components(
          const Ptr<DirichletProcessMixtureComponent> &merged,
          const Ptr<DirichletProcessMixtureComponent> &empty,
          const Ptr<DirichletProcessMixtureComponent> &split1,
          const Ptr<DirichletProcessMixtureComponent> &split2);

      // Set the mixing weights for the proposal.  See 'set_components' for the
      // meanings of 'split' and 'merged'.
      // Args:
      //   merged_mixing_weights: The vector of mixing weights for the entire
      //     model (after the merge, or before the split).  There should be no
      //     terminal element giving weights for 'all remaining' mixture
      //     components (which is how the mixing weight vector is returned by
      //     DirichletProcessMixtureModel), so the sum of merged_mixing_weights
      //     will typically be less than 1.  There _should_ be a terminal mixing
      //     weight for the single 'empty' component, however.
      //   split_mixing_weights: The vector of mixing weights for the entire
      //     split model (before the merge, or after the split).  There should
      //     be no terminal element giving weights for empty mixture components,
      //     so the sum of split_mixing_weights will typically be less than 1.
      //     Its size and its sum must match merged_mixing_weights.
      void set_mixing_weights(
          const Vector &merged_mixing_weights,
          const Vector &split_mixing_weights);

      void set_log_proposal_density_ratio(double log_ratio) {
        log_split_to_merge_probability_ratio_ = log_ratio;
      }

      int data_index_1() const {return data_index_1_;}
      int data_index_2() const {return data_index_2_;}

      const Ptr<DirichletProcessMixtureComponent> &merged() const {
        return merged_;
      }
      const Ptr<DirichletProcessMixtureComponent> &empty() const {
        return empty_;
      }
      const Ptr<DirichletProcessMixtureComponent> &split1() const {
        return split1_;
      }
      const Ptr<DirichletProcessMixtureComponent> &split2() const {
        return split2_;
      }

      double merged_mixing_weight() const;
      double split1_mixing_weight() const;
      double split2_mixing_weight() const;
      double empty_mixing_weight() const;

      const Vector &split_mixing_weights() const {
        return split_mixing_weights_;
      }
      const Vector &merged_mixing_weights() const {
        return merged_mixing_weights_;
      }

      double log_split_to_merge_probability_ratio() const {
        return log_split_to_merge_probability_ratio_;
      }

      bool is_merge() const { return type_ == Merge; }

      // Raises an error if any data elements have not been set.
      void check();

     private:
      ProposalType type_;

      // The indices (in the DirichletProcessMixtureModel) of the first and
      // second data points defining the split-merge move.
      int data_index_1_;
      int data_index_2_;

      Ptr<DirichletProcessMixtureComponent> merged_;
      Ptr<DirichletProcessMixtureComponent> empty_;
      Ptr<DirichletProcessMixtureComponent> split1_;
      Ptr<DirichletProcessMixtureComponent> split2_;

      // Holds the log of q(split | merge) / q(merge | split), where q is the
      // proposal density.
      double log_split_to_merge_probability_ratio_;

      // The mixing weights are of the same dimension.  In a merge move the
      // final mixing weight corresponds to the first empty component.
      Vector split_mixing_weights_;
      Vector merged_mixing_weights_;
    };

    //======================================================================
    class ProposalStrategy {
     public:
      virtual ~ProposalStrategy() {}

      // Propose a split move with two components.  The first contains data
      // point data_index_1, and the second contains data_index_2.  Both data
      // points must currently belong to the same mixture component.
      virtual Proposal propose_split(int data_index_1, int data_index_2,
                                     RNG &rng) = 0;

      // Propose to merge the components containing data_index_1 and
      // data_index_2, which must belong to different components.
      virtual Proposal propose_merge(int data_index_1, int data_index_2,
                                     RNG &rng) = 0;
    };

    //======================================================================
    // Proposes a split move by choosing a single observation to seed an MCMC
    // algorithm to generate parameters for the new split component.  The
    // component being split from (the "old" component) retains the same
    // parameters.  Observations are allocated between the old and new
    // components with probability proportional to their likelihood under each
    // component raised to a power between 0 and 1 called the "annealing
    // factor".  The mixing weights for the split components take the mixing
    // weight for the original component and split it in proportion to the
    // number of observations in each split.
    //
    // Proposes to merge components by combining data from two components into
    // whichever was arbitrarily labeled "component 1".
    class SingleObservationSplitStrategy
        : public ProposalStrategy {
     public:
      // Args:
      //   model: The model to be posterior sampled.
      //   annealing_factor: Observations are randomly allocated between
      //     components in a split move with probability proportional to
      //     f(y)^alpha, where f is the density of each component, and alpha is
      //     the annealing_factor.  The annealing factor balances between splits
      //     and merges.  Numbers close to 1 tend to yield more splits.  Numbers
      //     closer to zero tend to yield more merges.
      SingleObservationSplitStrategy(DirichletProcessMixtureModel *model,
                                     double annealing_factor = 1.0);

      Proposal propose_split(int data_index_1, int data_index_2,
                             RNG &rng) override;
      Proposal propose_merge(int data_index_1, int data_index_2,
                             RNG &rng) override;

      // Returns the log of the proposal density ratio for the split move.
      // I.e. the log of p(from merged to split) / p(from split to merged). The
      // procedure for proposing splits and merges is described elsewhere.
      // Please read the rest of this file if notions such as "seed observation"
      // are unclear.
      //
      // Args:
      //   split1: The component containing seed observation 1, after the split
      //     (or before the merge).
      //   split2: The component containing seed observation 2, after the split
      //     (or before the merge).
      //   merged: The component containing all the data from split1 and
      //     split2 after the merge (or before the split).
      //   empty: The first unpopulated mixture component after the merge (or
      //     before the split).
      //   empty_mixing_weight_proportion: The proportion of the total mixing
      //     weight (the sum of the mixing weights for split1 and split2)
      //     occupied by the empty component.
      //   log_allocation_probability: The log of the probability that the data
      //     would be split as observed between split1 and split2.  The two
      //     seed observations do not contribute to this probability.
      //   data_index_2: The index (in the global DP model) of the data point
      //     serving as the second seed observation.
      double split_log_proposal_density_ratio(
          const Proposal &proposal,
          double log_allocation_probability,
          int data_index_2) const;

      // Return a split proposal, initialized from an original merged component.
      // The returned proposal has its seeding data point assigned.  The seeding
      // data point is removed from the data set of elements owned by the
      // original_component.  (The data set is a copy.  The actual vector of data
      // in original_component is not modified here.)
      //
      // If the proposal is from "component 1" then its parameters are set to
      // the parameters of the original component.  If it is from "component 2"
      // then its parameters are set to a draw from the posterior distribution
      // given the single data element used to seed it.  The draw is
      // accomplished using a long-ish MCMC run starting from the parameters in
      // original_component.
      //
      // Args:
      //   original_component: The original component, for which a split
      //     is to be attempted.
      //   original_component_data_set: A std::set containing the data from
      //     original_component.
      //   data_index: The index of the data point (in the main Dirichlet
      //     process model, not in a particular mixture component) used to seed
      //     the split.  This data point will be removed from
      //     original_component_data_set.
      //   initialize_parameters: Set this to 'false' if creating component 1,
      //     whose parameters must match the original.  Set it to true if
      //     creating component 2, which will need new parameter values.
      //   rng: The random number generator to use for simulating the parameters
      //     of the split cluster.
      Ptr<DirichletProcessMixtureComponent> initialize_split_proposal(
          const Ptr<DirichletProcessMixtureComponent> &original_component,
          std::set<Ptr<Data>> &original_component_data_set,
          int data_index,
          bool initialize_parameters,
          RNG &rng);

      // Simulates parameters from their posterior distribution.
      void sample_parameters(DirichletProcessMixtureComponent &component);

      // Randomly assigns the data in 'data_set' to the two mixture components
      // according to their posterior probability in a two-component equally
      // weighted mixture.  Returns the log probability of the realized
      // assignment.
      double allocate_data_between_split_components(
          DirichletProcessMixtureComponent *split1,
          DirichletProcessMixtureComponent *split2,
          const std::set<Ptr<Data>> &data_set,
          RNG &rng) const;

      // The log probability that the data in the union of the data sets for two
      // mixture components would be allocated as observed.
      // Args:
      //   split1:  The first component.
      //   split2:  The second component.
      //   data_index_1: The index (in the data vector managed by the global
      //     DPMM) of the observation used to seed split1.  This observation
      //     must be left out of the probability calculation.
      //   data_index_2: The index (in the data vector managed by the global
      //     DPMM) of the observation used to seed split2. This observation
      //     must be left out of the probability calculation.
      double compute_log_partition_probability(
          const Ptr<DirichletProcessMixtureComponent> &split1,
          const Ptr<DirichletProcessMixtureComponent> &split2,
          int data_index_1,
          int data_index_2) const;

      // Computes the log of the probability that the data in 'component' would
      // be allocated as observed, in an equally weighted two-component mixture.
      // This function is used to implement 'compute_log_partition_probability'.
      //
      // Args:
      //   component: The component for which an allocation probability is
      //     desired.
      //   other_component: The other component in the mixture, with which
      //     'component' is competing for observations.
      //   data_index: The index (in the data vector managed by the global DPMM)
      //     of the observation used to seed component.  This observation must
      //     be left out of the probability calculation.
      double log_allocation_probability(
          const Ptr<DirichletProcessMixtureComponent> &component,
          const Ptr<DirichletProcessMixtureComponent> &other_component,
          int data_index) const;

      DirichletProcessMixtureModel *model_;
      const double annealing_factor_;
    };

  }  // namespace SplitMerge

}  // namespace BOOM

#endif //  BOOM_DIRICHLET_PROCESS_SPLIT_MERGE_PROPOSALS_HPP_
