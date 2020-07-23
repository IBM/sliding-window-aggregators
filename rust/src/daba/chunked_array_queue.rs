use std::collections::linked_list::Cursor as ChunkCursor;
use std::collections::LinkedList;
use std::mem::MaybeUninit;

/// An [Unrolled Linked List](https://en.wikipedia.org/wiki/Unrolled_linked_list).
pub struct ChunkedArrayQueue<Elem, const SIZE: usize> {
    chunks: LinkedList<Chunk<Elem, SIZE>>,
}

struct Chunk<Elem, const SIZE: usize> {
    elems: [MaybeUninit<Elem>; SIZE],
    front: usize,
    back: usize,
}

#[derive(Clone)]
pub(crate) struct Cursor<'i, Elem, const SIZE: usize> {
    /// A cursor pointing at a chunk
    chunk_cursor: ChunkCursor<'i, Chunk<Elem, SIZE>>,
    /// An index pointing at an element inside the chunk
    elem_index: usize,
}

impl<Elem, const SIZE: usize> Chunk<Elem, SIZE>
where
    Elem: Copy,
{
    fn new_middle() -> Self {
        Self {
            // Unsafe needed so we don't have to initialize every element of the array
            elems: unsafe { std::mem::MaybeUninit::uninit().assume_init() },
            front: SIZE / 2,
            back: SIZE / 2,
        }
    }
    fn new_front() -> Self {
        Self {
            elems: unsafe { std::mem::MaybeUninit::uninit().assume_init() },
            front: SIZE - 1,
            back: SIZE - 1,
        }
    }
    fn new_back() -> Self {
        Self {
            elems: unsafe { std::mem::MaybeUninit::uninit().assume_init() },
            front: 0,
            back: 0,
        }
    }
}

impl<Elem, const SIZE: usize> ChunkedArrayQueue<Elem, SIZE>
where
    Elem: Copy,
{
    pub(crate) fn new() -> Self {
        let chunk = Chunk::new_middle();
        let mut chunks = LinkedList::new();
        chunks.push_back(chunk);
        Self { chunks }
    }
    pub(crate) fn push_back(&mut self, v: Elem) {
        match self.chunks.back_mut() {
            Some(chunk) if chunk.back < SIZE - 1 => {
                chunk.back += 1;
                chunk.elems[chunk.back] = MaybeUninit::new(v);
            }
            _ => {
                self.chunks.push_back(Chunk::new_back());
                self.push_back(v)
            }
        }
    }
    pub(crate) fn push_front(&mut self, v: Elem) {
        match self.chunks.front_mut() {
            Some(chunk) if chunk.front > 0 => {
                chunk.front -= 1;
                chunk.elems[chunk.front] = MaybeUninit::new(v);
            }
            _ => {
                self.chunks.push_front(Chunk::new_front());
                self.push_front(v)
            }
        }
    }
    pub(crate) fn pop_back(&mut self) -> Option<Elem> {
        if let Some(chunk) = self.chunks.back_mut() {
            if chunk.front <= chunk.back {
                unsafe {
                    let val = chunk.elems[chunk.back].assume_init();
                    chunk.elems[chunk.back] = MaybeUninit::uninit().assume_init();
                    chunk.back -= 1;
                    if chunk.back == 0 {
                        self.chunks.pop_back();
                    }
                    Some(val)
                }
            } else {
                None
            }
        } else {
            None
        }
    }
    pub(crate) fn pop_front(&mut self) -> Option<Elem> {
        if let Some(chunk) = self.chunks.front_mut() {
            if chunk.front <= chunk.back {
                unsafe {
                    let val = chunk.elems[chunk.front].assume_init();
                    chunk.elems[chunk.front] = MaybeUninit::uninit().assume_init();
                    chunk.front += 1;
                    if chunk.front == SIZE {
                        self.chunks.pop_front();
                    }
                    Some(val)
                }
            } else {
                None
            }
        } else {
            None
        }
    }
    pub(crate) fn index_front(&mut self) -> Cursor<'_, Elem, SIZE> {
        let chunk_cursor = self.chunks.cursor_front();
        let elem_index = self.chunks.front().unwrap().front;
        Cursor {
            chunk_cursor,
            elem_index,
        }
    }
    pub(crate) fn index_back(&mut self) -> Cursor<'_, Elem, SIZE> {
        let chunk_cursor = self.chunks.cursor_back();
        let elem_index = self.chunks.back().unwrap().back;
        Cursor {
            chunk_cursor,
            elem_index,
        }
    }
}

impl<'i, Elem, const SIZE: usize> Cursor<'i, Elem, SIZE> {
    pub(crate) fn move_next(&mut self) {
        if self.elem_index + 1 < self.chunk_cursor.current().unwrap().front {
            self.elem_index += 1;
        } else {
            self.elem_index = 0;
            self.chunk_cursor.move_next()
        }
    }
    pub(crate) fn move_prev(&mut self) {
        if self.elem_index < self.chunk_cursor.current().unwrap().back {
            self.elem_index -= 1;
        } else {
            self.elem_index = SIZE - 1;
            self.chunk_cursor.move_prev()
        }
    }
    pub(crate) fn get(&mut self) -> &Elem {
        // OK because elem should always be initialized at this point
        unsafe { &self.chunk_cursor.current().unwrap().elems[self.elem_index].get_ref() }
    }
    pub(crate) fn set(&mut self, v: Elem) {
        // Cursors must be able to mutate elems. Using CursorMut is not possible,
        // since the cursor must also be cloneable. Therefore, unsafe is used here.
        unsafe {
            let ptr = self.chunk_cursor.current().unwrap() as *const Chunk<Elem, SIZE>
                as *mut Chunk<Elem, SIZE>;
            (*ptr).elems[self.elem_index] = MaybeUninit::new(v);
        }
    }
}

impl<'i, Elem, const SIZE: usize> PartialEq for Cursor<'i, Elem, SIZE> {
    fn eq(&self, other: &Self) -> bool {
        self.elem_index == other.elem_index
            && self.chunk_cursor.index() == other.chunk_cursor.index()
    }
}

#[test]
fn test() {
    let mut queue = ChunkedArrayQueue::<i32, 512>::new();
    queue.push_front(4);
    queue.push_front(4);
    queue.push_front(7);
    queue.push_back(1);
    assert_eq!(queue.pop_front(), Some(7));
    assert_eq!(queue.pop_front(), Some(4));
    assert_eq!(queue.pop_back(), Some(1));
    assert_eq!(queue.pop_front(), Some(4));
}
