//
// Created by WangJingYu on 2021/6/30.
//

#include <cvm/node/functor.h>
#include <cvm/node/node.h>
#include <cvm/node/reflection.h>
#include <cvm/node/structural_equal.h>
#include <cvm/runtime/registry.h>

#include <unordered_map>

namespace cvm {

bool ReflectionVTable::SEqualReduce(const Object* self, const Object* other,
                                    SEqualReducer equal) const {
  uint32_t tindex = self->type_index();
  if (tindex >= fsequal_reduce_.size() || fsequal_reduce_[tindex] == nullptr) {
    LOG(FATAL) << "TypeError: SEqualReduce of " << self->GetTypeKey()
               << " is not registered via CVM_REGISTER_NODE_TYPE."
               << " Did you forget to set _type_has_method_sequal_reduce=true?";
  }
  return fsequal_reduce_[tindex](self, other, equal);
}

/*!
 * \brief A non recursive stack based SEqual handler that can remaps vars.
 *
 * This handler pushes the Object equality cases into a stack, and
 * traverses the stack to expand the necessary children that need to be checked.
 *
 * The order of SEqual being called is the same as the order as if we
 * eagerly do recursive calls in SEqualReduce.
 */
class RemapVarSEqualHandler : public SEqualReducer::Handler {
 public:
  explicit RemapVarSEqualHandler(bool assert_mode) : assert_mode_(assert_mode) {}

  bool SEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars) final {
    // We cannot use check lhs.same_as(rhs) to check equality.
    // if we choose enable var remapping.
    //
    // Counter example below (%x, %y) are shared vars
    // between the two functions(possibly before/after rewriting).
    //
    // - function0: fn (%x, %y) { %x + %y }
    // - function1: fn (%y, %x) { %x + %y }
    //
    // Because we choose to enable var remapping,
    // %x is mapped to %y, and %y is mapped to %x
    // the body of the function no longer means the same thing.
    //
    // Take away: We can either choose only compare Var by address,
    // in which case we can use same_as for quick checking.
    // or we have to run deep comparison and avoid to use same_as checks.
    auto run = [=]() {
      if (!lhs.defined() && !rhs.defined()) return true;
      if (!lhs.defined() && rhs.defined()) return false;
      if (lhs.defined() && !rhs.defined()) return false;
      if (lhs->type_index() != rhs->type_index()) return false;
      auto it = equal_map_lhs_.find(lhs);
      if (it != equal_map_lhs_.end()) {
        return it->second.same_as(rhs);
      }
      if (equal_map_rhs_.count(rhs)) return false;
      // need to push to pending tasks in this case
      pending_tasks_.emplace_back(Task(lhs, rhs, map_free_vars));
      return true;
    };
    return CheckResult(run(), lhs, rhs);
  }

  void MarkGraphNode() final {
    // need to push to pending tasks in this case
    ICHECK(!allow_push_to_stack_ && !task_stack_.empty());
    task_stack_.back().graph_equal = true;
  }

  ObjectRef MapLhsToRhs(const ObjectRef& lhs) final {
    auto it = equal_map_lhs_.find(lhs);
    if (it != equal_map_lhs_.end()) return it->second;
    return ObjectRef(nullptr);
  }

  // Function that implements actual equality check.
  bool Equal(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars) {
    if (!lhs.defined() && !rhs.defined()) return true;
    task_stack_.clear();
    pending_tasks_.clear();
    equal_map_lhs_.clear();
    equal_map_rhs_.clear();
    if (!SEqualReduce(lhs, rhs, map_free_vars)) return false;
    ICHECK_EQ(pending_tasks_.size(), 1U);
    ICHECK(allow_push_to_stack_);
    task_stack_.emplace_back(std::move(pending_tasks_.back()));
    pending_tasks_.clear();
    return RunTasks();
  }

 protected:
  // Check the result.
  bool CheckResult(bool result, const ObjectRef& lhs, const ObjectRef& rhs) {
    if (assert_mode_ && !result) {
      LOG(FATAL) << "ValueError: Structural check failed, cased by\n"
                 << "lhs = " << lhs << "\nrhs= " << rhs;
    }
    return result;
  }
  /*!
   * \brief Run tasks util the stack reaches the stack begin
   * \return The checks we encountered throughout the process.
   */
  bool RunTasks() {
    while (task_stack_.size() != 0) {
      auto& entry = task_stack_.back();

      if (entry.children_expanded) {
        auto it = equal_map_lhs_.find(entry.lhs);
        if (it != equal_map_lhs_.end()) {
          ICHECK(it->second.same_as(entry.rhs));
        }
        if (entry.graph_equal) {
          equal_map_lhs_[entry.lhs] = entry.lhs;
          equal_map_rhs_[entry.rhs] = entry.rhs;
        }
        task_stack_.pop_back();
      } else {
        entry.children_expanded = true;
        ICHECK_EQ(pending_tasks_.size(), 0U);
        allow_push_to_stack_ = false;
        if (!DispatchSEqualReduce(entry.lhs, entry.rhs, entry.map_free_vars)) return false;
        allow_push_to_stack_ = true;

        while (pending_tasks_.size() != 0) {
          task_stack_.emplace_back(std::move(pending_tasks_.back()));
          pending_tasks_.pop_back();
        }
      }
    }
    return true;
  }

  bool DispatchSEqualReduce(const ObjectRef& lhs, const ObjectRef& rhs, bool map_free_vars) {
    auto compute = [=]() {
      ICHECK(lhs.defined() && rhs.defined() && lhs->type_index() == rhs->type_index());
      auto it = equal_map_lhs_.find(lhs);
      if (it != equal_map_rhs_.end()) {
        return it->second.same_as(rhs);
      }
      if (equal_map_rhs_.count(rhs)) return false;
      // Run reduce check for free nodes.
      return vtable_->SEqualReduce(lhs.get(), rhs.get(), SEqualReducer(this, map_free_vars));
    };
    return CheckResult(compute(), lhs, rhs);
  }

 private:
  /*! \brief The lhs operand to be compared. */
  struct Task {
    ObjectRef lhs;
    ObjectRef rhs;
    bool map_free_vars;
    bool children_expanded{false};
    bool graph_equal{false};

    Task() = default;
    Task(ObjectRef lhs, ObjectRef rhs, bool map_free_vars)
        : lhs(lhs), rhs(rhs), map_free_vars(map_free_vars) {}
  };

  // list of pending tasks to be pushed to the stack.
  std::vector<Task> pending_tasks_;
  // Internal task stack to executed the task.
  std::vector<Task> task_stack_;
  // Whether we allow push to stack.
  bool allow_push_to_stack_{true};
  // If in assert mode, must return true, and will throw error otherwise
  bool assert_mode_{false};
  // reflection vtable
  ReflectionVTable* vtable_ = ReflectionVTable::Global();
  // map from lhs to rhs
  std::unordered_map<ObjectRef, ObjectRef, ObjectPtrHash, ObjectPtrEqual> equal_map_lhs_;
  // map from rhs to lhs
  std::unordered_map<ObjectRef, ObjectRef, ObjectPtrHash, ObjectPtrEqual> equal_map_rhs_;
};

CVM_REGISTER_GLOBAL("node.StructuralEqual")
    .set_body_typed([](const ObjectRef& lhs, const ObjectRef& rhs, bool assert_mode,
                       bool map_free_vars) {
      return RemapVarSEqualHandler(assert_mode).Equal(lhs, rhs, map_free_vars);
    });

bool StructuralEqual::operator()(const ObjectRef& lhs, const ObjectRef& rhs) const {
  return RemapVarSEqualHandler(false).Equal(lhs, rhs, false);
}

}  // namespace cvm