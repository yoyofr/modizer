// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#ifndef ASMJIT_DISABLE_COMPILER

// [Dependencies - AsmJit]
#include "../base/assembler.h"
#include "../base/compiler.h"
#include "../base/context_p.h"
#include "../base/cpuinfo.h"
#include "../base/intutil.h"
#include "../base/logger.h"

// [Dependencies - C]
#include <cstdarg>

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Constants]
// ============================================================================

static const char noName[1] = { '\0' };
enum { kBaseCompilerDefaultLookAhead = 64 };

// ============================================================================
// [asmjit::Compiler - Construction / Destruction]
// ============================================================================

Compiler::Compiler(Runtime* runtime) :
  CodeGen(runtime),
  _assembler(nullptr),
  _nodeFlowId(0),
  _nodeFlags(0),
  _maxLookAhead(kBaseCompilerDefaultLookAhead),
  _targetVarMapping(nullptr),
  _firstNode(nullptr),
  _lastNode(nullptr),
  _cursor(nullptr),
  _func(nullptr),
  _varZone(4096 - kZoneOverhead),
  _stringZone(4096 - kZoneOverhead),
  _localConstZone(4096 - kZoneOverhead),
  _localConstPool(&_localConstZone),
  _globalConstPool(&_baseZone) {}

Compiler::~Compiler() {
  reset(true);

  if (_assembler)
    delete _assembler;
}

// ============================================================================
// [asmjit::Compiler - Clear / Reset]
// ============================================================================

void Compiler::reset(bool releaseMemory) {
  // CodeGen members.
  _baseAddress = kNoBaseAddress;
  _instOptions = 0;
  _error = kErrorOk;

  _baseZone.reset(releaseMemory);

  // Compiler members.
  _nodeFlowId = 0;
  _nodeFlags = 0;

  if (_assembler)
    _assembler->reset(releaseMemory);

  _firstNode = nullptr;
  _lastNode = nullptr;

  _cursor = nullptr;
  _func = nullptr;

  _localConstPool.reset();
  _globalConstPool.reset();

  _localConstPoolLabel.reset();
  _globalConstPoolLabel.reset();

  _varZone.reset(releaseMemory);
  _stringZone.reset(releaseMemory);
  _localConstZone.reset(releaseMemory);

  _targetList.reset(releaseMemory);
  _varList.reset(releaseMemory);
}

// ============================================================================
// [asmjit::Compiler - Node Management]
// ============================================================================

Node* Compiler::setCursor(Node* node) {
  Node* old = _cursor;
  _cursor = node;
  return old;
}

Node* Compiler::addNode(Node* node) {
  ASMJIT_ASSERT(node);
  ASMJIT_ASSERT(!node->_prev);
  ASMJIT_ASSERT(!node->_next);

  if (!_cursor) {
    if (!_firstNode) {
      _firstNode = node;
      _lastNode = node;
    }
    else {
      node->_next = _firstNode;
      _firstNode->_prev = node;
      _firstNode = node;
    }
  }
  else {
    Node* prev = _cursor;
    Node* next = _cursor->_next;

    node->_prev = prev;
    node->_next = next;

    prev->_next = node;
    if (next)
      next->_prev = node;
    else
      _lastNode = node;
  }

  _cursor = node;
  return node;
}

Node* Compiler::addNodeBefore(Node* node, Node* ref) {
  ASMJIT_ASSERT(node);
  ASMJIT_ASSERT(!node->_prev);
  ASMJIT_ASSERT(!node->_next);
  ASMJIT_ASSERT(ref);

  Node* prev = ref->_prev;
  Node* next = ref;

  node->_prev = prev;
  node->_next = next;

  next->_prev = node;
  if (prev)
    prev->_next = node;
  else
    _firstNode = node;

  return node;
}

Node* Compiler::addNodeAfter(Node* node, Node* ref) {
  ASMJIT_ASSERT(node);
  ASMJIT_ASSERT(!node->_prev);
  ASMJIT_ASSERT(!node->_next);
  ASMJIT_ASSERT(ref);

  Node* prev = ref;
  Node* next = ref->_next;

  node->_prev = prev;
  node->_next = next;

  prev->_next = node;
  if (next)
    next->_prev = node;
  else
    _lastNode = node;

  return node;
}

static ASMJIT_INLINE void BaseCompiler_nodeRemoved(Compiler*, Node* node_) {
  if (node_->isJmpOrJcc()) {
    JumpNode* node = static_cast<JumpNode*>(node_);
    TargetNode* target = node->getTarget();

    // Disconnect.
    JumpNode** pPrev = &target->_from;
    for (;;) {
      ASMJIT_ASSERT(*pPrev);
      JumpNode* current = *pPrev;

      if (!current)
        break;

      if (current == node) {
        *pPrev = node->_jumpNext;
        break;
      }

      pPrev = &current->_jumpNext;
    }

    target->subNumRefs();
  }
}

Node* Compiler::removeNode(Node* node) {
  Node* prev = node->_prev;
  Node* next = node->_next;

  if (_firstNode == node)
    _firstNode = next;
  else
    prev->_next = next;

  if (_lastNode == node)
    _lastNode  = prev;
  else
    next->_prev = prev;

  node->_prev = nullptr;
  node->_next = nullptr;

  if (_cursor == node)
    _cursor = prev;
  BaseCompiler_nodeRemoved(this, node);

  return node;
}

void Compiler::removeNodes(Node* first, Node* last) {
  if (first == last) {
    removeNode(first);
    return;
  }

  Node* prev = first->_prev;
  Node* next = last->_next;

  if (_firstNode == first)
    _firstNode = next;
  else
    prev->_next = next;

  if (_lastNode == last)
    _lastNode  = prev;
  else
    next->_prev = prev;

  Node* node = first;
  for (;;) {
    next = node->getNext();
    ASMJIT_ASSERT(next);

    node->_prev = nullptr;
    node->_next = nullptr;

    if (_cursor == node)
      _cursor = prev;
    BaseCompiler_nodeRemoved(this, node);

    if (node == last)
      break;
    node = next;
  }
}

// ============================================================================
// [asmjit::Compiler - Align]
// ============================================================================

AlignNode* Compiler::newAlign(uint32_t mode, uint32_t offset) {
  AlignNode* node = newNode<AlignNode>(mode, offset);
  if (!node)
    goto _NoMemory;
  return node;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return nullptr;
}

AlignNode* Compiler::addAlign(uint32_t mode, uint32_t offset) {
  AlignNode* node = newAlign(mode, offset);
  if (!node)
    return nullptr;
  return static_cast<AlignNode*>(addNode(node));
}

// ============================================================================
// [asmjit::Compiler - Target]
// ============================================================================

TargetNode* Compiler::newTarget() {
  TargetNode* node = newNode<TargetNode>(
    OperandUtil::makeLabelId(static_cast<uint32_t>(_targetList.getLength())));

  if (!node || _targetList.append(node) != kErrorOk)
    goto _NoMemory;
  return node;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return nullptr;
}

TargetNode* Compiler::addTarget() {
  TargetNode* node = newTarget();
  if (!node)
    return nullptr;
  return static_cast<TargetNode*>(addNode(node));
}

// ============================================================================
// [asmjit::Compiler - Label]
// ============================================================================

Error Compiler::_newLabel(Label* dst) {
  dst->_init_packed_op_sz_b0_b1_id(kOperandTypeLabel, 0, 0, 0, kInvalidValue);
  dst->_init_packed_d2_d3(0, 0);

  TargetNode* node = newTarget();
  if (!node)
    goto _NoMemory;

  dst->_label.id = node->getLabelId();
  return kErrorOk;

_NoMemory:
  return setError(kErrorNoHeapMemory);
}

Error Compiler::bind(const Label& label) {
  uint32_t index = label.getId();
  ASMJIT_ASSERT(index < _targetList.getLength());

  addNode(_targetList[index]);
  return kErrorOk;
}

// ============================================================================
// [asmjit::Compiler - Embed]
// ============================================================================

EmbedNode* Compiler::newEmbed(const void* data, uint32_t size) {
  EmbedNode* node;

  if (size > EmbedNode::kInlineBufferSize) {
    void* clonedData = _stringZone.alloc(size);
    if (!clonedData)
      goto _NoMemory;

    if (data)
      ::memcpy(clonedData, data, size);
    data = clonedData;
  }

  node = newNode<EmbedNode>(const_cast<void*>(data), size);
  if (!node)
    goto _NoMemory;
  return node;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return nullptr;
}

EmbedNode* Compiler::addEmbed(const void* data, uint32_t size) {
  EmbedNode* node = newEmbed(data, size);
  if (!node)
    return node;
  return static_cast<EmbedNode*>(addNode(node));
}

// ============================================================================
// [asmjit::Compiler - Comment]
// ============================================================================

CommentNode* Compiler::newComment(const char* str) {
  CommentNode* node;

  if (str && str[0]) {
    str = _stringZone.sdup(str);
    if (!str)
      goto _NoMemory;
  }

  node = newNode<CommentNode>(str);
  if (!node)
    goto _NoMemory;
  return node;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return nullptr;
}

CommentNode* Compiler::addComment(const char* str) {
  CommentNode* node = newComment(str);
  if (!node)
    return nullptr;
  return static_cast<CommentNode*>(addNode(node));
}

CommentNode* Compiler::comment(const char* fmt, ...) {
  char buf[256];
  char* p = buf;

  if (fmt) {
    *p++ = ';';
    *p++ = ' ';

    va_list ap;
    va_start(ap, fmt);
    p += vsnprintf(p, 254, fmt, ap);
    va_end(ap);
  }

  p[0] = '\n';
  p[1] = '\0';

  return addComment(fmt);
}

// ============================================================================
// [asmjit::Compiler - Hint]
// ============================================================================

HintNode* Compiler::newHint(Var& var, uint32_t hint, uint32_t value) {
  if (var.getId() == kInvalidValue)
    return nullptr;
  VarData* vd = getVd(var);

  HintNode* node = newNode<HintNode>(vd, hint, value);
  if (!node)
    goto _NoMemory;
  return node;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return nullptr;
}

HintNode* Compiler::addHint(Var& var, uint32_t hint, uint32_t value) {
  if (var.getId() == kInvalidValue)
    return nullptr;

  HintNode* node = newHint(var, hint, value);
  if (!node)
    return nullptr;
  return static_cast<HintNode*>(addNode(node));
}

// ============================================================================
// [asmjit::Compiler - Vars]
// ============================================================================

VarData* Compiler::_newVd(uint32_t type, uint32_t size, uint32_t c, const char* name) {
  VarData* vd = reinterpret_cast<VarData*>(_varZone.alloc(sizeof(VarData)));
  if (!vd)
    goto _NoMemory;

  vd->_name = noName;
  vd->_id = OperandUtil::makeVarId(static_cast<uint32_t>(_varList.getLength()));
  vd->_contextId = kInvalidValue;

  if (name && name[0] != '\0') {
    vd->_name = _stringZone.sdup(name);
  }

  vd->_type = static_cast<uint8_t>(type);
  vd->_class = static_cast<uint8_t>(c);
  vd->_flags = 0;
  vd->_priority = 10;

  vd->_state = kVarStateUnused;
  vd->_regIndex = kInvalidReg;
  vd->_isStack = false;
  vd->_isMemArg = false;
  vd->_isCalculated = false;
  vd->_saveOnUnuse = false;
  vd->_modified = false;
  vd->_reserved0 = 0;
  vd->_alignment = static_cast<uint8_t>(IntUtil::iMin<uint32_t>(size, 64));

  vd->_size = size;
  vd->_homeMask = 0;

  vd->_memOffset = 0;
  vd->_memCell = nullptr;

  vd->rReadCount = 0;
  vd->rWriteCount = 0;
  vd->mReadCount = 0;
  vd->mWriteCount = 0;

  vd->_va = nullptr;

  if (_varList.append(vd) != kErrorOk)
    goto _NoMemory;
  return vd;

_NoMemory:
  setError(kErrorNoHeapMemory);
  return nullptr;
}

void Compiler::alloc(Var& var) {
  addHint(var, kVarHintAlloc, kInvalidValue);
}

void Compiler::alloc(Var& var, uint32_t regIndex) {
  addHint(var, kVarHintAlloc, regIndex);
}

void Compiler::alloc(Var& var, const Reg& reg) {
  addHint(var, kVarHintAlloc, reg.getRegIndex());
}

void Compiler::save(Var& var) {
  addHint(var, kVarHintSave, kInvalidValue);
}

void Compiler::spill(Var& var) {
  addHint(var, kVarHintSpill, kInvalidValue);
}

void Compiler::unuse(Var& var) {
  addHint(var, kVarHintUnuse, kInvalidValue);
}

uint32_t Compiler::getPriority(Var& var) const {
  if (var.getId() == kInvalidValue)
    return kInvalidValue;

  VarData* vd = getVdById(var.getId());
  return vd->getPriority();
}

void Compiler::setPriority(Var& var, uint32_t priority) {
  if (var.getId() == kInvalidValue)
    return;

  if (priority > 255)
    priority = 255;

  VarData* vd = getVdById(var.getId());
  vd->_priority = static_cast<uint8_t>(priority);
}

bool Compiler::getSaveOnUnuse(Var& var) const {
  if (var.getId() == kInvalidValue)
    return false;

  VarData* vd = getVdById(var.getId());
  return static_cast<bool>(vd->_saveOnUnuse);
}

void Compiler::setSaveOnUnuse(Var& var, bool value) {
  if (var.getId() == kInvalidValue)
    return;

  VarData* vd = getVdById(var.getId());
  vd->_saveOnUnuse = value;
}

void Compiler::rename(Var& var, const char* name) {
  if (var.getId() == kInvalidValue)
    return;

  VarData* vd = getVdById(var.getId());
  vd->_name = noName;

  if (name && name[0] != '\0') {
    vd->_name = _stringZone.sdup(name);
  }
}

// ============================================================================
// [asmjit::Compiler - Assembler]
// ============================================================================

Assembler* Compiler::getAssembler() {
  Assembler* a = _assembler;

  if (a) {
    a->reset(false);
  }
  else {
    a = _newAssembler();
    _assembler = a;
  }

#ifndef ASMJIT_DISABLE_LOGGER
  Logger* logger = _logger;
  if (logger)
    a->setLogger(logger);
#endif // !ASMJIT_DISABLE_LOGGER

  a->setBaseAddress(_baseAddress);
  a->setFeatures(_features);

  return a;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
