// Copyright 2018 Google LLC. All Rights Reserved.
/*
  Copyright (C) 2007 Steven L. Scott

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
#ifndef BOOM_SPD_STORAGE_HPP
#define BOOM_SPD_STORAGE_HPP

#include <Models/DataTypes.hpp>
#include <functional>

namespace BOOM {
  namespace SPD {

    // Abstract base class for storing the different incarnations of a
    // symmetric positive definite matrix.  The class keeps track of
    // whether or not the currently stored data is current by placing
    // observers in other Storage objects.  When the user sets the
    // data in a Storage object, it signals the list of observers
    // watching it, so they know that their data needs to be updated.
    class Storage {
     public:
      // Args:
      //   current: A flag indicating whether the data held by storage
      //     is current
      Storage(bool current = false);
      Storage(const Storage &rhs);
      virtual ~Storage();
      virtual Storage * clone() const = 0;

      // Returns the length of one side of the stored matrix.
      virtual uint dim() const = 0;

      // The storage capacity requirements of either the full matrix
      // (minimial = false), or the triangle (minimial = true).
      virtual uint size(bool minimal = true) const;

      // Is the stored data current?
      bool current() const;

      // Signal any observers that a change has been made.
      void signal();

      // Marks the stored data as current.
      void set_current();

      // Creates an observer that can be placed in another Storage
      // object using add_observer().
      std::function<void(void)> create_observer();

      // Adds an from another Storage object.
      void add_observer(const std::function<void(void)> &f);

     private:
      bool current_;
      void observe_changes();

      std::vector<std::function<void(void)> >signals_;
    };

    //------------------------------------------------------------
    class SpdStorage;
    class CholStorage : public Storage{
    public:
      CholStorage();
      CholStorage(const SpdMatrix &S);
      CholStorage(const CholStorage &rhs);
      CholStorage * clone() const override;
      uint dim() const override;
      void set(const Matrix &L, bool sig = true);
      void refresh(const SpdStorage &);
      const Matrix & value() const;
    private:
      Matrix L;
    };

    //------------------------------------------------------------
    class SpdStorage : public Storage{
    public:
      SpdStorage();
      SpdStorage(const SpdMatrix &S);
      SpdStorage(const SpdStorage &S);
      SpdStorage * clone() const override;
      uint dim() const override;
      const SpdMatrix & value() const;
      void set(const SpdMatrix &, bool sig = true);
      void refresh_from_chol(const CholStorage&, bool inv = false);
      void refresh_from_inv(const SpdStorage &, CholStorage &);
    private:
      SpdMatrix sig_;
    };
  }  // namespace SPD
  //____________________________________________________________

  // This class stores a SpdMatrix matrix in several different formats.  It
  // keeps both the original matrix (though of as a variance matrix),
  // its inverse (ivar), as well as the cholesky triangles for these
  // two matrices.  This is extravagant, but it prevents excessive
  // computation.
  class SpdData
    : public DataTraits<Spd>
  {
  public:
    SpdData(uint n, double diag=1.0, bool ivar = false);
    SpdData(const SpdMatrix &S, bool ivar = false);
    SpdData(const SpdData &rhs);
    SpdData * clone() const override;

    virtual uint size(bool minimal = true) const;
    virtual uint dim() const;
    ostream & display(ostream &out) const override;

    const SpdMatrix & value() const override;
    void set(const SpdMatrix &v, bool sig = true) override;

    const SpdMatrix & var() const;
    const SpdMatrix & ivar() const;
    const Matrix & var_chol() const;
    const Matrix & ivar_chol() const;
    double ldsi() const;

    void set_var(const SpdMatrix &, bool signal = true);
    void set_ivar(const SpdMatrix &, bool signal = true);
    void set_var_chol(const Matrix &L, bool signal = true);
    void set_ivar_chol(const Matrix &L, bool signal = true);

    // Args:
    //   sd:  A vector of standard deviations to go along the diagonal.
    //   L:  The lower cholesky triangle for a correlation matrix.
    void set_S_Rchol(const Vector &sd, const Matrix &L);

    // Check that the desired representation contains the desired
    // data.  If not then find and refresh from the current
    // representation.
    void ensure_ivar_current() const;
    void ensure_var_current() const;
    void ensure_var_chol_current() const;
    void ensure_ivar_chol_current() const;

  private:
    mutable std::shared_ptr<SPD::SpdStorage> var_;
    mutable std::shared_ptr<SPD::SpdStorage> ivar_;
    mutable std::shared_ptr<SPD::CholStorage> var_chol_;
    mutable std::shared_ptr<SPD::CholStorage> ivar_chol_;

    // Points to the current representation: variance, inverse
    // variance, cholesky of variance, cholesky of inverse.
    std::shared_ptr<SPD::Storage> current_rep_;

    // Create observers among all the available storage modes.
    void setup_storage();
  };
}  // namespace BOOM
#endif// BOOM_SPD_STORAGE_HPP
