"""homm1.core - semantically blind primitives.

Rules of the layer:
  * imports stdlib only (never inputs/, never the model);
  * knows formats and bytes, never what a label means;
  * the moment a helper wants two inputs at once, it belongs in the model.
"""
