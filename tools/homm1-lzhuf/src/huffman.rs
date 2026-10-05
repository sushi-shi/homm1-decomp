//! The adaptive Huffman tree shared by the encoder and decoder.
//!
//! Retail has two copies of the update and rebuild routines: the VC6-compiled
//! `UpdateEncoderTree`/`ReconstructEncoderTree` and the Watcom-family assembly
//! `UpdateDecoderTree`/`ReconstructDecoderTree`. Both operate on the same
//! `son`/`freq`/`prnt` globals with the same arithmetic, so one implementation
//! serves both directions here.

use crate::tables::{CHARACTER_COUNT, FREQUENCY_SENTINEL, MAX_FREQUENCY, ROOT, TREE_SIZE};

/// Node frequencies, children and parents, laid out as in the game.
///
/// `son[node]` is the left child of an internal node (the right child is
/// `son[node] + 1`), or `TREE_SIZE + symbol` for a leaf. `parent` has
/// `TREE_SIZE + CHARACTER_COUNT` entries: `parent[TREE_SIZE + symbol]` is the
/// leaf node that holds `symbol`. `freq[TREE_SIZE]` is a sentinel that stops
/// the sorted-order scan in [`AdaptiveTree::update`].
#[derive(Clone)]
pub(crate) struct AdaptiveTree {
    pub(crate) freq: [u16; TREE_SIZE + 1],
    pub(crate) son: [u16; TREE_SIZE],
    pub(crate) parent: [u16; TREE_SIZE + CHARACTER_COUNT],
}

impl AdaptiveTree {
    /// Okumura's `StartHuff` state, which the game stores as `initialSon`,
    /// `initialFrequency` and `initialParent` and copies before every call.
    pub(crate) fn new() -> Self {
        let mut tree = Self {
            freq: [0; TREE_SIZE + 1],
            son: [0; TREE_SIZE],
            parent: [0; TREE_SIZE + CHARACTER_COUNT],
        };
        for symbol in 0..CHARACTER_COUNT {
            tree.freq[symbol] = 1;
            tree.son[symbol] = node(symbol + TREE_SIZE);
            tree.parent[symbol + TREE_SIZE] = node(symbol);
        }
        let mut child = 0;
        for internal in CHARACTER_COUNT..TREE_SIZE {
            tree.freq[internal] = tree.freq[child] + tree.freq[child + 1];
            tree.son[internal] = node(child);
            tree.parent[child] = node(internal);
            tree.parent[child + 1] = node(internal);
            child += 2;
        }
        tree.freq[TREE_SIZE] = FREQUENCY_SENTINEL;
        tree.parent[ROOT] = 0;
        tree
    }

    /// The leaf node holding `symbol`.
    pub(crate) fn leaf(&self, symbol: usize) -> usize {
        usize::from(self.parent[symbol + TREE_SIZE])
    }

    /// Counts one occurrence of `symbol`, keeping frequencies sorted by
    /// swapping each incremented node past its equals.
    pub(crate) fn update(&mut self, symbol: usize) {
        if self.freq[ROOT] == MAX_FREQUENCY {
            self.reconstruct();
        }
        let mut current = self.leaf(symbol);
        loop {
            self.freq[current] = self.freq[current].wrapping_add(1);
            // The game keeps the incremented count in a `short` and compares
            // it with the unsigned neighbours after integer promotion.
            let count = i32::from(self.freq[current] as i16);
            let mut swap = current + 1;
            if count > i32::from(self.freq[swap]) {
                loop {
                    swap += 1;
                    if count <= i32::from(self.freq[swap]) {
                        break;
                    }
                }
                swap -= 1;
                self.freq[current] = self.freq[swap];
                self.freq[swap] = count as u16;

                let moved = usize::from(self.son[current]);
                self.set_parent(moved, swap);
                let displaced = usize::from(self.son[swap]);
                self.son[swap] = node(moved);
                self.set_parent(displaced, current);
                self.son[current] = node(displaced);
                current = swap;
            }
            current = usize::from(self.parent[current]);
            if current == 0 {
                break;
            }
        }
    }

    /// Points a child (and, for an internal node, its right sibling) at
    /// `parent`.
    fn set_parent(&mut self, child: usize, parent: usize) {
        self.parent[child] = node(parent);
        if child < TREE_SIZE {
            self.parent[child + 1] = node(parent);
        }
    }

    /// Halves every leaf frequency and rebuilds the internal nodes, as
    /// `ReconstructEncoderTree`/`ReconstructDecoderTree` do once the root
    /// reaches [`MAX_FREQUENCY`].
    fn reconstruct(&mut self) {
        let mut leaves = 0;
        for index in 0..TREE_SIZE {
            if usize::from(self.son[index]) >= TREE_SIZE {
                self.freq[leaves] = u32::from(self.freq[index]).div_ceil(2) as u16;
                self.son[leaves] = self.son[index];
                leaves += 1;
            }
        }

        let mut child = 0;
        for internal in CHARACTER_COUNT..TREE_SIZE {
            let frequency = self.freq[child].wrapping_add(self.freq[child + 1]);
            self.freq[internal] = frequency;
            let mut slot = internal - 1;
            while frequency < self.freq[slot] {
                slot -= 1;
            }
            slot += 1;
            self.freq.copy_within(slot..internal, slot + 1);
            self.freq[slot] = frequency;
            self.son.copy_within(slot..internal, slot + 1);
            self.son[slot] = node(child);
            child += 2;
        }

        for index in 0..TREE_SIZE {
            let child = usize::from(self.son[index]);
            if child >= TREE_SIZE {
                self.parent[child] = node(index);
            } else {
                self.set_parent(child, index);
            }
        }
    }
}

/// Node indices and leaf codes are below `TREE_SIZE + CHARACTER_COUNT` (941).
fn node(index: usize) -> u16 {
    debug_assert!(index < TREE_SIZE + CHARACTER_COUNT);
    index as u16
}
