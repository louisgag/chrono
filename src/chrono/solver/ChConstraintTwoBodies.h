//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2010 Alessandro Tasora
// Copyright (c) 2013 Project Chrono
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file at the top level of the distribution
// and at http://projectchrono.org/license-chrono.txt.
//

#ifndef CHCONSTRAINTTWOBODIES_H
#define CHCONSTRAINTTWOBODIES_H

#include "chrono/solver/ChConstraintTwo.h"
#include "chrono/solver/ChVariablesBody.h"

namespace chrono {

/// This class inherits from the base ChConstraintTwo(),
/// that implements the functionality for a constraint between
/// a couple of two objects of type ChVariablesBody().

class ChApi ChConstraintTwoBodies : public ChConstraintTwo {
    CH_RTTI(ChConstraintTwoBodies, ChConstraintTwo)

  protected:
    /// The [Cq_a] jacobian of the constraint
    ChMatrixNM<double, 1, 6> Cq_a;
    /// The [Cq_b] jacobian of the constraint
    ChMatrixNM<double, 1, 6> Cq_b;

    // Auxiliary data: will be used by iterative constraint solvers:

    /// The [Eq_a] product [Eq_a]=[invM_a]*[Cq_a]'
    ChMatrixNM<double, 6, 1> Eq_a;
    /// The [Eq_a] product [Eq_b]=[invM_b]*[Cq_b]'
    ChMatrixNM<double, 6, 1> Eq_b;

  public:
    /// Default constructor
    ChConstraintTwoBodies() {}

    /// Construct and immediately set references to variables
    ChConstraintTwoBodies(ChVariablesBody* mvariables_a, ChVariablesBody* mvariables_b) {
        SetVariables(mvariables_a, mvariables_b);
    }

    /// Copy constructor
    ChConstraintTwoBodies(const ChConstraintTwoBodies& other) : ChConstraintTwo(other) {
        Cq_a = other.Cq_a;
        Cq_b = other.Cq_b;
        Eq_a = other.Eq_a;
        Eq_b = other.Eq_b;
    }

    virtual ~ChConstraintTwoBodies() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChConstraintTwoBodies* Clone() const override { return new ChConstraintTwoBodies(*this); }

    /// Assignment operator: copy from other object
    ChConstraintTwoBodies& operator=(const ChConstraintTwoBodies& other);

    /// Access jacobian matrix
    virtual ChMatrix<double>* Get_Cq_a() { return &Cq_a; }
    /// Access jacobian matrix
    virtual ChMatrix<double>* Get_Cq_b() { return &Cq_b; }

    /// Access auxiliary matrix (ex: used by iterative solvers)
    virtual ChMatrix<double>* Get_Eq_a() { return &Eq_a; }
    /// Access auxiliary matrix (ex: used by iterative solvers)
    virtual ChMatrix<double>* Get_Eq_b() { return &Eq_b; }

    /// Set references to the constrained objects, each of ChVariablesBody type,
    /// automatically creating/resizing jacobians if needed.
    /// If variables aren't from ChVariablesBody class, an assert failure happens.
    void SetVariables(ChVariables* mvariables_a, ChVariables* mvariables_b);

    /// This function updates the following auxiliary data:
    ///  - the Eq_a and Eq_b matrices
    ///  - the g_i product
    /// This is often called by solvers at the beginning
    /// of the solution process.
    /// Most often, inherited classes won't need to override this.
    virtual void Update_auxiliary();

    ///  This function must computes the product between
    /// the row-jacobian of this constraint '[Cq_i]' and the
    /// vector of variables, 'v'. that is    CV=[Cq_i]*v
    ///  This is used for some iterative solvers.
    virtual double Compute_Cq_q() {
        double ret = 0;

        if (variables_a->IsActive())
            for (int i = 0; i < 6; i++)
                ret += Cq_a.ElementN(i) * variables_a->Get_qb().ElementN(i);

        if (variables_b->IsActive())
            for (int i = 0; i < 6; i++)
                ret += Cq_b.ElementN(i) * variables_b->Get_qb().ElementN(i);

        return ret;
    }

    ///  This function must increment the vector of variables
    /// 'v' with the quantity [invM]*[Cq_i]'*deltal,that is
    ///   v+=[invM]*[Cq_i]'*deltal  or better: v+=[Eq_i]*deltal
    ///  This is used for some iterative solvers.
    virtual void Increment_q(const double deltal) {
        if (variables_a->IsActive())
            for (int i = 0; i < 6; i++)
                variables_a->Get_qb()(i) += Eq_a.ElementN(i) * deltal;

        if (variables_b->IsActive())
            for (int i = 0; i < 6; i++)
                variables_b->Get_qb()(i) += Eq_b.ElementN(i) * deltal;
    }

    /// Computes the product of the corresponding block in the
    /// system matrix by 'vect', and add to 'result'.
    /// NOTE: the 'vect' vector must already have
    /// the size of the total variables&constraints in the system; the procedure
    /// will use the ChVariable offsets (that must be already updated) to know the
    /// indexes in result and vect;
    virtual void MultiplyAndAdd(double& result, const ChMatrix<double>& vect) const {
        int off_a = variables_a->GetOffset();
        int off_b = variables_b->GetOffset();

        if (variables_a->IsActive())
            for (int i = 0; i < 6; i++)
                result += vect(off_a + i) * Cq_a.ElementN(i);

        if (variables_b->IsActive())
            for (int i = 0; i < 6; i++)
                result += vect(off_b + i) * Cq_b.ElementN(i);
    }

    /// Computes the product of the corresponding transposed blocks in the
    /// system matrix (ie. the TRANSPOSED jacobian matrix C_q') by 'l', and add to 'result'.
    /// NOTE: the 'result' vector must already have
    /// the size of the total variables&constraints in the system; the procedure
    /// will use the ChVariable offsets (that must be already updated) to know the
    /// indexes in result and vect;
    virtual void MultiplyTandAdd(ChMatrix<double>& result, double l) {
        int off_a = variables_a->GetOffset();
        int off_b = variables_b->GetOffset();

        if (variables_a->IsActive())
            for (int i = 0; i < 6; i++)
                result(off_a + i) += Cq_a.ElementN(i) * l;

        if (variables_b->IsActive())
            for (int i = 0; i < 6; i++)
                result(off_b + i) += Cq_b.ElementN(i) * l;
    }

    /// Puts the two jacobian parts into the 'insrow' row of a sparse matrix,
    /// where both portions of the jacobian are shifted in order to match the
    /// offset of the corresponding ChVariable.The same is done
    /// on the 'insrow' column, so that the sparse matrix is kept symmetric.
    /// This is used only by the ChSolverSimplex solver (iterative solvers
    /// don't need to know jacobians explicitly)
    virtual void Build_Cq(ChSparseMatrix& storage, int insrow) {
        if (variables_a->IsActive())
            storage.PasteMatrix(&Cq_a, insrow, variables_a->GetOffset());
        if (variables_b->IsActive())
            storage.PasteMatrix(&Cq_b, insrow, variables_b->GetOffset());
    }
    virtual void Build_CqT(ChSparseMatrix& storage, int inscol) {
        if (variables_a->IsActive())
            storage.PasteTranspMatrix(&Cq_a, variables_a->GetOffset(), inscol);
        if (variables_b->IsActive())
            storage.PasteTranspMatrix(&Cq_b, variables_b->GetOffset(), inscol);
    }

    /// Method to allow deserializing a persistent binary archive (ex: a file)
    /// into transient data.
    virtual void StreamIN(ChStreamInBinary& mstream);

    /// Method to allow serializing transient data into a persistent
    /// binary archive (ex: a file).
    virtual void StreamOUT(ChStreamOutBinary& mstream);
};

}  // end namespace chrono

#endif