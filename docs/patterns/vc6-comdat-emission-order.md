# VC6 COMDAT function order: member templates last

Measured with the pinned HoMM1 Buka VC6 SP5 compiler under
`/Od /Ob1 /GX /MT /G5` on variants of `BASE/Audio`, compiled outside the
build and compared by the order of their `.text` COMDAT sections. LINK keeps
an object's COMDATs in that order.

| Node destructor shape | COMDAT order |
| --- | --- |
| `inline ~Node();` defined later in the TU (or at its end) | `~Node`, the `RefPtr` members |
| implicit destructor of a plain struct | `~Node`, the `RefPtr` members |
| implicit destructor of a class template instance | `~Node<T>`, the `RefPtr` members |
| user-declared destructor in a class template | the `RefPtr` members, `~Node<T>` |

- A non-template function that is not inlined, and the compiler-generated
  destructor, are emitted before the class template members, wherever they
  are defined.
- Members of class template specializations are emitted after them, grouped
  by specialization in instantiation order: a specialization is instantiated
  when a complete type is first needed (a data member of that type, not a
  `static` member declaration).
- A user-declared member of a class template is emitted with the other
  template members.

Retail `BASE/Audio` ends with the `RefPtr<OutputStream>`, `RefPtr<SampleSource>`
and `RefPtr<AudioDevice>` members, then the sample-list node destructor
(0x00469f80). Only the last row produces that order, so the node is a class
template with a declared destructor (`AudiereNode<sample>`). Its
`RefPtr<OutputStream>` member is declared before `AudiereMusic` and
`AudiereDevice`, which fixes the specialization order.
